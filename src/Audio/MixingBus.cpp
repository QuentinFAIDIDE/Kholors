#include "MixingBus.h"
#include "DataSource.h"
#include "MixbusDataSource.h"
#include "SamplePlayer.h"
#include "UnitConverter.h"

#include <cstdint>
#include <iostream>
#include <memory>

// initialize the MixingBus, as well as Thread and audio inherited
// behaviours.
MixingBus::MixingBus(ActivityManager &am)
    : Thread("Mixbus Loader Thread"), activityManager(am), playCursor(0), totalFrameLength(0), numChannels(2),
      isPlaying(false), loopingToggledOn(false), loopSectionStartFrame(22050), loopSectionEndFrame(44100),
      forwardFFT(FREQVIEW_SAMPLE_FFT_ORDER), uiState(am.getAppState().getUiState())
{
    // initialize format manager
    formatManager.registerBasicFormats();

    // set master bus gain
    masterGain.setGainDecibels(0.0f);
    // make sure it's smoothing changes
    masterGain.setRampDurationSeconds(DSP_GAIN_SMOOTHING_RAMP_SEC);

    // warn if smoothing is disabled
    if (!masterGain.isSmoothing())
    {
        std::cerr << "Unable to set master gain smoothing" << std::endl;
    }

    lastDrawnCursor = 0;

    // create instance of mixbusDataSource
    mixbusDataSource = std::make_shared<MixbusDataSource>();

    // start the background thread that makes malloc/frees
    startThread();
}

MixingBus::~MixingBus()
{
    // stop thread with a 4sec timeout to kill it
    stopThread(4000);

    // delete all tracks
    for (size_t i = 0; i < tracks.size(); i++)
    {
        auto track = tracks.getUnchecked(i);
        if (track != nullptr)
        {
            // this will unset the buffer
            track->releaseResources();

            // this will delete the sample if we're the only holder
            track.reset();
        }
    }

    // delete all buffers
    checkForBuffersToFree();
}

std::shared_ptr<MixbusDataSource> MixingBus::getMixbusDataSource()
{
    return mixbusDataSource;
}

bool MixingBus::taskHandler(std::shared_ptr<Task> task)
{
    auto sc = std::dynamic_pointer_cast<SampleCreateTask>(task);
    if (sc != nullptr && !sc->isCompleted() && !sc->hasFailed())
    {
        addSample(sc);
        return true;
    }

    auto sd = std::dynamic_pointer_cast<SampleDeletionTask>(task);
    if (sd != nullptr && !sd->isCompleted() && !sd->hasFailed())
    {
        deleteSample(sd);
        return true;
    }

    auto restoreTask = std::dynamic_pointer_cast<SampleRestoreTask>(task);
    if (restoreTask != nullptr && !restoreTask->isCompleted() && !restoreTask->hasFailed())
    {
        restoreSample(restoreTask);
        return true;
    }

    auto moveTask = std::dynamic_pointer_cast<SampleMovingTask>(task);
    if (moveTask != nullptr && !moveTask->isCompleted() && !moveTask->hasFailed())
    {
        if (tracks[moveTask->id] != nullptr)
        {
            tracks[moveTask->id]->move(tracks[moveTask->id]->getEditingPosition() + moveTask->dragDistance);
            moveTask->setCompleted(true);
            moveTask->setFailed(false);

            // we need to tell arrangement are to update
            std::shared_ptr<SampleUpdateTask> sut =
                std::make_shared<SampleUpdateTask>(moveTask->id, tracks[moveTask->id]);
            activityManager.broadcastNestedTaskNow(sut);

            trackRepaintCallback();
        }
        else
        {
            moveTask->setCompleted(true);
            moveTask->setFailed(true);
        }
        return true;
    }

    auto timeCropTask = std::dynamic_pointer_cast<SampleTimeCropTask>(task);
    if (timeCropTask != nullptr && !timeCropTask->isCompleted() && !timeCropTask->hasFailed())
    {
        cropSample(timeCropTask);
        return true;
    }

    auto freqCropTask = std::dynamic_pointer_cast<SampleFreqCropTask>(task);
    if (freqCropTask != nullptr && !freqCropTask->isCompleted() && !freqCropTask->hasFailed())
    {
        cropSample(freqCropTask);
        return true;
    }

    auto selectUpdate = std::dynamic_pointer_cast<SelectionChangingTask>(task);
    if (selectUpdate != nullptr && selectUpdate->isCompleted())
    {
        mixbusDataSource->updateSelectedTracks(selectUpdate->newSelectedTracks);
        return true;
    }

    std::shared_ptr<PlayStateUpdateTask> playUpdate = std::dynamic_pointer_cast<PlayStateUpdateTask>(task);
    if (playUpdate != nullptr && !playUpdate->isCompleted())
    {
        if (playUpdate->requestingStateBroadcast)
        {
            playUpdate->isCurrentlyPlaying = isPlaying;
            playUpdate->setCompleted(true);
            activityManager.broadcastNestedTaskNow(playUpdate);
            return true;
        }

        // if we reach here, it's not a request for update, it's a request for
        // execution of play/pause state change. To prevent wasting resources,
        // we won't treat and broadcast state update for update that won't
        // change the state.
        if (playUpdate->shouldPlay)
        {
            if (isPlaying)
            {
                playUpdate->setFailed(true);
                // we stop broadcast as no other TaskListener should
                // respond to this cursed useless task.
                return true;
            }
            else
            {
                startPlayback();
                playUpdate->setCompleted(true);
                playUpdate->isCurrentlyPlaying = true;
                activityManager.broadcastNestedTaskNow(playUpdate);
                return true;
            }
        }
        else
        {
            if (!isPlaying)
            {
                if (playUpdate->shouldResetPosition)
                {
                    {
                        const juce::ScopedLock lock(mixbusMutex);
                        playCursor = 0;
                        // spread update to each track/sample who
                        // have their own position copy
                        setNextReadPosition(playCursor);
                    }
                    playUpdate->setCompleted(true);
                    playUpdate->isCurrentlyPlaying = false;
                    // no need to broadcast this completed task or to resume
                    // it broadcasting as the playback state didn't really change.
                    return true;
                }
                else
                {
                    playUpdate->setFailed(true);
                    return true;
                }
            }
            else
            {
                stopPlayback();
                if (playUpdate->shouldResetPosition)
                {
                    const juce::ScopedLock lock(mixbusMutex);
                    playCursor = 0;
                    // spread update to each track/sample who
                    // have their own position copy
                    setNextReadPosition(playCursor);
                }
                playUpdate->setCompleted(true);
                playUpdate->isCurrentlyPlaying = false;
                activityManager.broadcastNestedTaskNow(playUpdate);
                return true;
            }
        }
    }

    auto loopToggleTask = std::dynamic_pointer_cast<LoopToggleTask>(task);
    if (loopToggleTask != nullptr && !loopToggleTask->isCompleted())
    {
        if (!loopToggleTask->requestingStateBroadcast)
        {
            loopingToggledOn = loopToggleTask->shouldLoop;
        }
        loopToggleTask->isCurrentlyLooping = loopingToggledOn;
        loopToggleTask->setCompleted(true);
        activityManager.broadcastNestedTaskNow(loopToggleTask);
        return true;
    }

    auto loopMovingTask = std::dynamic_pointer_cast<LoopMovingTask>(task);
    if (loopMovingTask != nullptr && !loopMovingTask->isCompleted())
    {
        if (!loopMovingTask->isBroadcastRequest)
        {
            loopSectionStartFrame = loopMovingTask->currentLoopBeginFrame;
            loopSectionEndFrame = loopMovingTask->currentLoopEndFrame;
        }
        else
        {
            loopMovingTask->currentLoopBeginFrame = loopSectionStartFrame;
            loopMovingTask->currentLoopEndFrame = loopSectionEndFrame;
        }
        loopMovingTask->setCompleted(true);
        activityManager.broadcastNestedTaskNow(loopMovingTask);
        return true;
    }

    auto fadeChangeTask = std::dynamic_pointer_cast<SampleFadeChange>(task);
    if (fadeChangeTask != nullptr && !fadeChangeTask->isCompleted())
    {
        // do nothing if the sample doesn't exist
        if (fadeChangeTask->sampleId < 0 || fadeChangeTask->sampleId >= tracks.size() ||
            tracks[fadeChangeTask->sampleId] == nullptr)
        {
            return false;
        }

        // broadcast result if it's what we need to do
        if (fadeChangeTask->isBroadcastRequest)
        {
            fadeChangeTask->currentFadeInFrameLen = tracks[fadeChangeTask->sampleId]->getFadeInLength();
            fadeChangeTask->currentFadeOutFrameLen = tracks[fadeChangeTask->sampleId]->getFadeOutLength();
            fadeChangeTask->setCompleted(true);
            activityManager.broadcastNestedTaskNow(fadeChangeTask);
            return true;
        }

        bool hasUpdated = false;
        if (!fadeChangeTask->onlyFadeOut)
        {
            bool fadeInUpdate =
                tracks[fadeChangeTask->sampleId]->setFadeInLength(fadeChangeTask->currentFadeInFrameLen);
            hasUpdated = fadeInUpdate;

            fadeChangeTask->currentFadeInFrameLen = tracks[fadeChangeTask->sampleId]->getFadeInLength();
        }

        if (!fadeChangeTask->onlyFadeIn)
        {
            bool fadeOutUpdate =
                tracks[fadeChangeTask->sampleId]->setFadeOutLength(fadeChangeTask->currentFadeOutFrameLen);
            hasUpdated = hasUpdated || fadeOutUpdate;
            fadeChangeTask->currentFadeOutFrameLen = tracks[fadeChangeTask->sampleId]->getFadeOutLength();
        }

        fadeChangeTask->setCompleted(true);
        if (hasUpdated)
        {
            fadeChangeTask->setFailed(false);
        }
        else
        {
            fadeChangeTask->setFailed(true);
        }

        activityManager.broadcastNestedTaskNow(fadeChangeTask);
        return true;
    }

    auto gainChangeTask = std::dynamic_pointer_cast<SampleGainChange>(task);
    if (gainChangeTask != nullptr && !gainChangeTask->isCompleted() && !gainChangeTask->hasFailed())
    {
        // do nothing if the sample doesn't exist
        if (gainChangeTask->sampleId < 0 || gainChangeTask->sampleId >= tracks.size() ||
            tracks[gainChangeTask->sampleId] == nullptr)
        {
            return false;
        }

        if (!gainChangeTask->isBroadcastRequest)
        {
            tracks[gainChangeTask->sampleId]->setDbGain(gainChangeTask->currentDbGain);
        }
        gainChangeTask->currentDbGain = tracks[gainChangeTask->sampleId]->getDbGain();

        gainChangeTask->setCompleted(true);
        activityManager.broadcastNestedTaskNow(gainChangeTask);
        return true;
    }

    return false;
}

void MixingBus::cropSample(std::shared_ptr<SampleTimeCropTask> task)
{
    if (tracks[task->id] != nullptr)
    {

        int initialFadeIn = tracks[task->id]->getFadeInLength();
        int initialFadeOut = tracks[task->id]->getFadeOutLength();

        if (task->movingBeginning)
        {
            tracks[task->id]->tryMovingStart(task->dragDistance);
        }
        else
        {
            tracks[task->id]->tryMovingEnd(task->dragDistance);
        }

        task->setCompleted(true);
        task->setFailed(false);

        // we need to tell arrangement are to update
        std::shared_ptr<SampleUpdateTask> sut = std::make_shared<SampleUpdateTask>(task->id, tracks[task->id]);
        activityManager.broadcastNestedTaskNow(sut);

        // if the fade changed due to resize, broadcast the change
        int finalFadeIn = tracks[task->id]->getFadeInLength();
        int finalFadeOut = tracks[task->id]->getFadeOutLength();

        if (initialFadeIn != finalFadeIn || initialFadeOut != finalFadeOut)
        {
            auto fadeUpdate =
                std::make_shared<SampleFadeChange>(task->id, initialFadeIn, initialFadeOut, finalFadeIn, finalFadeOut);
            activityManager.broadcastNestedTaskNow(fadeUpdate);
        }

        trackRepaintCallback();
    }
    else
    {
        task->setCompleted(true);
        task->setFailed(true);
    }
}

void MixingBus::cropSample(std::shared_ptr<SampleFreqCropTask> task)
{
    if (tracks[task->id] != nullptr)
    {
        if (task->isLowPass)
        {
            tracks[task->id]->setLowPassFreq(task->finalFrequency);
        }
        else
        {
            tracks[task->id]->setHighPassFreq(task->finalFrequency);
        }

        task->setCompleted(true);
        task->setFailed(false);

        // we need to tell arrangement are to update
        std::shared_ptr<SampleUpdateTask> sut = std::make_shared<SampleUpdateTask>(task->id, tracks[task->id]);
        activityManager.broadcastNestedTaskNow(sut);

        trackRepaintCallback();
    }
    else
    {
        task->setCompleted(true);
        task->setFailed(true);
    }
}

bool MixingBus::filePathsValid(const juce::StringArray &files)
{
    // TODO: implement this
    return true;
}

void MixingBus::startPlayback()
{
    if (!isPlaying)
    {
        setNextReadPosition(playCursor);
        isPlaying = true;
    }
}

void MixingBus::stopPlayback()
{
    if (isPlaying)
    {
        isPlaying = false;
    }
}

bool MixingBus::isCursorPlaying() const
{
    return isPlaying;
}

void MixingBus::addSample(std::shared_ptr<SampleCreateTask> importTask)
{
    if (importTask->isDuplication())
    {
        duplicateTrack(importTask);
    }
    else
    {
        importNewFile(importTask);
    }
}

void MixingBus::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    const juce::ScopedLock lock(mixbusMutex);
    // prepare all inputs
    for (size_t i = 0; i < tracks.size(); i++)
    {
        if (tracks.getUnchecked(i) != nullptr)
        {
            tracks.getUnchecked(i)->prepareToPlay(samplesPerBlockExpected, sampleRate);
        }
    }

    // prepare all master bus effects if necessary

    // for the limiter we need to fill a spec object
    currentAudioSpec.sampleRate = sampleRate;
    currentAudioSpec.maximumBlockSize = juce::uint32(samplesPerBlockExpected);
    currentAudioSpec.numChannels = numChannels;

    masterGain.prepare(currentAudioSpec);

    // allocate/free memory around in the background thread
    notify();
}

void MixingBus::releaseResources()
{
    const juce::ScopedLock lock(mixbusMutex);

    // call all inputs releaseResources
    for (size_t i = 0; i < tracks.size(); i++)
    {
        if (tracks.getUnchecked(i) != nullptr)
        {
            tracks.getUnchecked(i)->releaseResources();
        }
    }

    masterGain.reset();

    // clear output buffer
    audioThreadBuffer.setSize(2, 0);
}

void MixingBus::getNextAudioBlock(const juce::AudioSourceChannelInfo &bufferToFill)
{
    // the mixing code here was initially based on the MixerAudioSource one from
    // Juce.

    // get scoped lock of the reentering mutex
    const juce::ScopedLock sl(mixbusMutex);

    // if there is more then one input track and we are playing
    if (tracks.size() > 0 && isPlaying)
    {

        // get if possible a pointer to the set of selected tracks
        std::set<size_t> *selectedTracks = mixbusDataSource->getLockedSelectedTracks();

        // ensure selected audio buffer has necessary space
        if (selectedTracks != nullptr)
        {
            // initialize buffer
            audioThreadSelectionBuffer.setSize(juce::jmax(1, bufferToFill.buffer->getNumChannels()),
                                               bufferToFill.buffer->getNumSamples(), false, false, true);
            audioThreadSelectionBuffer.clear();
        }

        // get a pointer to a new processed input buffer from first source
        // we will append into this one to mix tracks together
        if (tracks.getUnchecked(0) != nullptr)
        {
            tracks.getUnchecked(0)->getNextAudioBlock(bufferToFill);

            // if the track is currently selected sum its volume
            if (selectedTracks != nullptr && selectedTracks->find(0) != selectedTracks->end())
            {
                // append it to the initial one
                for (int chan = 0; chan < bufferToFill.buffer->getNumChannels(); chan++)
                {
                    audioThreadSelectionBuffer.addFrom(chan, 0, (*(bufferToFill.buffer)), chan,
                                                       bufferToFill.startSample, bufferToFill.numSamples);
                }
            }
        }
        else
        {
            bufferToFill.clearActiveBufferRegion();
        }

        if (tracks.size() > 1)
        {
            // initialize buffer
            audioThreadBuffer.setSize(juce::jmax(1, bufferToFill.buffer->getNumChannels()),
                                      bufferToFill.buffer->getNumSamples(), false, false, true);

            // create a new getNextAudioBlock request that
            // will use our MixingBus buffer to pull
            // block to append to the previous buffer
            juce::AudioSourceChannelInfo copyBufferDest(&audioThreadBuffer, 0, bufferToFill.numSamples);

            // for each input source
            for (size_t i = 1; i < tracks.size(); i++)
            {
                if (tracks.getUnchecked(i) == nullptr)
                {
                    continue;
                }
                // get the next audio block in the buffer
                tracks.getUnchecked(i)->getNextAudioBlock(copyBufferDest);
                // abort whenever the buffer is empty
                if (audioThreadBuffer.getNumSamples() != 0)
                {
                    // append it to the initial one
                    for (int chan = 0; chan < bufferToFill.buffer->getNumChannels(); chan++)
                    {
                        bufferToFill.buffer->addFrom(chan, bufferToFill.startSample, audioThreadBuffer, chan, 0,
                                                     bufferToFill.numSamples);
                    }

                    // if the track is currently selected sum its volume
                    if (selectedTracks != nullptr && selectedTracks->find(i) != selectedTracks->end())
                    {
                        // append it to the initial one
                        for (int chan = 0; chan < bufferToFill.buffer->getNumChannels(); chan++)
                        {
                            audioThreadSelectionBuffer.addFrom(chan, 0, audioThreadBuffer, chan, 0,
                                                               bufferToFill.numSamples);
                        }
                    }
                }
            }
        }

        // create context to apply dsp effects
        juce::dsp::AudioBlock<float> audioBlockRef(*bufferToFill.buffer, bufferToFill.startSample);
        juce::dsp::ProcessContextReplacing<float> context(audioBlockRef);

        // apply master gain
        masterGain.process(context);

        // let's spread the master volume to the vu meter
        std::pair<float, float> masterVolume;
        masterVolume.first = UnitConverter::dbFromBufferChannel(bufferToFill, 0);
        masterVolume.second = UnitConverter::dbFromBufferChannel(bufferToFill, 1);
        vuMeterVolumes[VUMETER_ID_MASTER] = masterVolume;

        // this buffer audio source info allow the dbFromBufferChannel function to work on
        // the appropriate range
        juce::AudioSourceChannelInfo selectionBufferInfos(&audioThreadSelectionBuffer, 0, bufferToFill.numSamples);

        std::pair<float, float> selectionVolume;
        selectionVolume.first = UnitConverter::dbFromBufferChannel(selectionBufferInfos, 0);
        selectionVolume.second = UnitConverter::dbFromBufferChannel(selectionBufferInfos, 1);
        vuMeterVolumes[VUMETER_ID_SELECTED] = selectionVolume;

        // send all the vu meter values to the data source
        mixbusDataSource->swapVuMeterValues(vuMeterVolumes);

        // ensure we freed the selected tracks lock !
        mixbusDataSource->releaseSelectedTracks();
    }
    else
    {
        // if there's no tracks or we're not playing, clear output
        bufferToFill.clearActiveBufferRegion();
    }

    // if we have been playing, update cursor
    if (isPlaying)
    {
        playCursor += bufferToFill.numSamples;
        mixbusDataSource->setPosition(playCursor);
        // wake up the background thread if we need to do any redrawing
        if (abs(playCursor - lastDrawnCursor) > FREQVIEW_MIN_REDRAW_DISTANCE_FRAMES)
        {
            notify();
        }
    }

    // loop if this buffer covers the end of the loop section
    if (loopingToggledOn && loopSectionStartFrame != loopSectionEndFrame && loopSectionEndFrame >= playCursor &&
        loopSectionEndFrame <= playCursor + bufferToFill.numSamples)
    {
        setNextReadPosition(loopSectionStartFrame);
    }
}

// background thread content for allocating stuff
void MixingBus::run()
{
    // wait 500ms in a loop unless notify is called
    while (!threadShouldExit())
    {
        // check if we need to ask for a redraw to move cursor
        checkForCursorRedraw();
        // check for buffers to free
        checkForBuffersToFree();
        // do we need to stop playback because the cursor is not in bounds ?
        pauseIfCursorNotInBound();
        // wait untill next thread iteration
        wait(500);
    }
}

void MixingBus::checkForCursorRedraw()
{
    // if we were notified to redraw, do it
    if (abs(lastDrawnCursor - playCursor) > FREQVIEW_MIN_REDRAW_DISTANCE_FRAMES)
    {
        lastDrawnCursor = playCursor;
        trackRepaintCallback();
    }
}

void MixingBus::checkForBuffersToFree()
{
    // inspired by LoopingAudioSampleBuffer juce tutorial

    // reminder: no need to lock for buffers reads here since no
    // write happens outside of the current
    // background thread (that launches this procedure)

    // for each buffer where we store audio samples for
    for (auto i = buffers.size(); --i >= 0;)
    {
        // get it
        BufferPtr buffer(buffers.getUnchecked(i));
        // if it's only referenced here and in the buffer list
        // meaning it's not used in a SamplePlayer
        if (buffer->getReferenceCount() == 2)
            // free it
            buffers.remove(i);
    }
}

void MixingBus::importNewFile(std::shared_ptr<SampleCreateTask> task)
{
    // inspired by LoopingAudioSampleBuffer juce tutorial

    std::string pathToOpen = task->getFilePath();
    int64_t desiredPosition = task->getPosition();

    std::cout << "Preparing to load file at path: " << pathToOpen << std::endl;
    // create file object
    juce::File file(pathToOpen);
    // create reader object
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    // if the reader can read our file
    if (reader.get() != nullptr)
    {
        // sample duration in seconds
        auto duration = (float)reader->lengthInSamples / reader->sampleRate;

        std::cout << "The file has a length of " << duration << "secs" << std::endl;

        // only load it if below our global constant maximum sample duration
        if (duration < SAMPLE_MAX_DURATION_SEC && reader->lengthInSamples > SAMPLE_MIN_DURATION_FRAMES)
        {
            // allocate a buffer
            BufferPtr newBuffer =
                new ReferenceCountedBuffer(file.getFileName(), (int)reader->numChannels, (int)reader->lengthInSamples);

            // do the actual reading
            reader->read(newBuffer->getAudioSampleBuffer(), 0, (int)reader->lengthInSamples, 0, true, true);

            // TODO: compute hash of sample, and do not save those
            // which already exists. Use a function like getBufferFromHash
            // to set the new samplePlayer to the already existing sample.
            // HINT: File class can give us a hash already ! :)

            // create a new sample player
            std::shared_ptr<SamplePlayer> newSample = std::make_shared<SamplePlayer>(desiredPosition);
            try
            {
                // assign buffer to the new SamplePlayer
                newSample->setBuffer(newBuffer, forwardFFT);
                // if the memory allocation fail, abort with error
            }
            catch (std::bad_alloc &ba)
            {
                std::cout << "Unable to allocate memory to perform FFT" << std::endl;
                std::shared_ptr<NotificationTask> notif = std::make_shared<NotificationTask>(
                    juce::String("Unable to allocate memory to load: ") + pathToOpen);
                activityManager.broadcastTask(notif);
                newSample.reset();
                task->setFailed(true);
                return;
            }

            // get a scoped lock for the buffer array
            int newTrackIndex;
            {
                const juce::ScopedLock lock(mixbusMutex);

                // add new SamplePlayer to the tracks list (positionable audio
                // sources)
                if (task->reuseNewId)
                {
                    tracks.set(task->getAllocatedIndex(), newSample);
                    newTrackIndex = task->getAllocatedIndex();
                }
                else
                {
                    tracks.add(newSample);
                    newTrackIndex = tracks.size() - 1;
                }
                // add the buffer the array of audio buffers and set its position
                buffers.add(newBuffer);
                tracks[newTrackIndex]->setNextReadPosition(playCursor);
            }

            task->setAllocatedIndex(newTrackIndex);
            task->setCompleted(true);
            task->setFailed(false);

            // this is instructing to update the view
            std::shared_ptr<SampleDisplayTask> displayTask = std::make_shared<SampleDisplayTask>(newSample, task);
            activityManager.broadcastNestedTaskNow(displayTask);

            // this is instructing to record a count for file import
            std::shared_ptr<ImportFileCountTask> fcTask = std::make_shared<ImportFileCountTask>(pathToOpen);
            activityManager.broadcastNestedTaskNow(fcTask);

            // this make the arrangement area widget repaint.
            // TODO: replace with repaint from SampleDisplayTask
            trackRepaintCallback();
        }
        else
        {
            // notify user about sample being too long to be loaded
            if (duration >= SAMPLE_MAX_DURATION_SEC)
            {
                std::shared_ptr<NotificationTask> notif =
                    std::make_shared<NotificationTask>(juce::String("Max is ") + juce::String(SAMPLE_MAX_DURATION_SEC) +
                                                       juce::String("s, sample was not loaded: ") + pathToOpen);
                activityManager.broadcastTask(notif);
            }
            if (reader->lengthInSamples <= SAMPLE_MIN_DURATION_FRAMES)
            {
                std::shared_ptr<NotificationTask> notif = std::make_shared<NotificationTask>(
                    juce::String("min is ") + juce::String(SAMPLE_MIN_DURATION_FRAMES) +
                    juce::String("audio samples, sample was not loaded: ") + pathToOpen);
                activityManager.broadcastTask(notif);
            }
            task->setFailed(true);
            return;
        }
    }
    else
    {
        // if the file reader doesn't want to read, notify an error
        std::shared_ptr<NotificationTask> notif =
            std::make_shared<NotificationTask>(juce::String("Unable to read: ") + pathToOpen);
        activityManager.broadcastTask(notif);

        task->setFailed(true);
        return;
    }
}

void MixingBus::setNextReadPosition(juce::int64 nextReadPosition)
{
    // update play cursor
    playCursor = nextReadPosition;
    mixbusDataSource->setPosition(playCursor);

    // tell all samplePlayers to update positions
    for (size_t i = 0; i < tracks.size(); i++)
    {
        if (tracks.getUnchecked(i) != nullptr)
        {
            // tell the track to reconsider its position
            tracks.getUnchecked(i)->setNextReadPosition(nextReadPosition);
        }
    }
}

juce::int64 MixingBus::getNextReadPosition() const
{
    // TODO: simply returns total frame position
    return playCursor;
}

juce::int64 MixingBus::getTotalLength() const
{
    // TODO: simply returns total length of the entire track in frames
    return totalFrameLength;
}

bool MixingBus::isLooping() const
{
    // TODO: always return false, we don't allow looping the whole track
    //       maybe this can be a feature for later. We could also loop only
    //       between some loop marks in the future, even though it look like
    //       it's not the intended use for this callback.
    return false;
}

void MixingBus::setLooping(bool)
{
    // TODO
}

void MixingBus::pauseIfCursorNotInBound()
{
    // TODO
}

// note that it includes deleted tracks in the count
size_t MixingBus::getNumTracks() const
{
    return tracks.size();
}

std::shared_ptr<SamplePlayer> MixingBus::getTrack(int index) const
{
    // NOTE: We never delete SamplePlayers in the tracks array or insert mid-array
    // to prevent using a lock when reading.

    // get pointer to the value at index
    return tracks.getUnchecked(index);
}

void MixingBus::setTrackRepaintCallback(std::function<void()> f)
{
    trackRepaintCallback = f;
}

void MixingBus::deleteSample(std::shared_ptr<SampleDeletionTask> deletionTask)
{
    if (deletionTask->id < 0 || deletionTask->id >= tracks.size() || tracks[deletionTask->id] == nullptr)
    {
        deletionTask->setCompleted(true);
        deletionTask->setFailed(true);
        return;
    }

    // we save a reference to this sample in the tasks list to restore it if
    // users wants to.
    deletionTask->deletedSample = tracks[deletionTask->id];

    // clear this sample
    {
        const juce::ScopedLock lock(mixbusMutex);
        tracks.set(deletionTask->id, std::shared_ptr<SamplePlayer>(nullptr));
    }

    // clear the activityManager view
    std::shared_ptr<SampleDeletionDisplayTask> displayTask =
        std::make_shared<SampleDeletionDisplayTask>(deletionTask->id);
    activityManager.broadcastNestedTaskNow(displayTask);

    deletionTask->setCompleted(true);
    deletionTask->setFailed(false);
}

void MixingBus::restoreSample(std::shared_ptr<SampleRestoreTask> task)
{
    if (task->id < 0 || task->id >= tracks.size() || tracks[task->id] != nullptr || task->sampleToRestore == nullptr)
    {
        task->setCompleted(true);
        task->setFailed(true);
        return;
    }

    tracks.set(task->id, task->sampleToRestore);

    std::shared_ptr<SampleRestoreDisplayTask> displayTask =
        std::make_shared<SampleRestoreDisplayTask>(task->id, task->sampleToRestore);
    activityManager.broadcastNestedTaskNow(displayTask);

    task->setCompleted(true);
    task->setFailed(false);
}

void MixingBus::duplicateTrack(std::shared_ptr<SampleCreateTask> task)
{
    // Note: this runs from the background thread

    if (task->getDuplicateTargetId() < 0 || task->getDuplicateTargetId() >= tracks.size())
    {

        task->setFailed(true);
        return;
    }

    // get a reference to the buffer to duplicate
    std::shared_ptr<SamplePlayer> targetToDuplicate = tracks[task->getDuplicateTargetId()];

    if (targetToDuplicate == nullptr)
    {
        task->setFailed(true);
        return;
    }

    // we first backup the original sample high and low pass freqs for return feature to reverse the task
    float lowPassFreq = targetToDuplicate->getLowPassFreq();
    float highPassFreq = targetToDuplicate->getHighPassFreq();
    task->setFilterInitialFrequencies(highPassFreq, lowPassFreq);

    // we also save the original length
    task->setSampleInitialLength(targetToDuplicate->getLength());

    std::shared_ptr<SamplePlayer> newSample = nullptr;
    int newTrackIndex = -1;

    if (task->getDuplicationType() == DUPLICATION_TYPE_COPY_AT_POSITION)
    {
        newSample = targetToDuplicate->createDuplicate(task->getPosition());
    }
    else if (task->getDuplicationType() == DUPLICATION_TYPE_SPLIT_AT_FREQUENCY)
    {
        newSample = targetToDuplicate->splitAtFrequency(task->getSplitFrequency());
    }
    else if (task->getDuplicationType() == DUPLICATION_TYPE_SPLIT_AT_POSITION)
    {
        newSample = targetToDuplicate->splitAtPosition((int)task->getPosition());
    }

    if (newSample == nullptr)
    {
        task->setFailed(true);
        return;
    }

    // get a scoped lock for the buffer array
    {
        // DEADLOCK HERE WHEN ADDING A SAMPLE NEAR LOOP END
        const juce::ScopedLock lock(mixbusMutex);

        if (task->reuseNewId)
        {
            newTrackIndex = task->getAllocatedIndex();
            tracks.set(task->getAllocatedIndex(), newSample);
        }
        else
        {
            // add new SamplePlayer to the tracks list (positionable audio
            // sources)
            tracks.add(newSample);
            newTrackIndex = tracks.size() - 1;
        }
        tracks[newTrackIndex]->setNextReadPosition(playCursor);
    }

    task->setAllocatedIndex(newTrackIndex);
    task->setCompleted(true);

    std::shared_ptr<SampleDisplayTask> displayTask = std::make_shared<SampleDisplayTask>(newSample, task);
    activityManager.broadcastNestedTaskNow(displayTask);

    // TODO: this date from before tasks were implement.
    // See if it's not better to repaint directly based
    // on ArrangementArea receiving SampleDisplayTask
    trackRepaintCallback();
}