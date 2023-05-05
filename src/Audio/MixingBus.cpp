#include "MixingBus.h"
#include "SamplePlayer.h"

#include <cstdint>
#include <iostream>
#include <memory>

// initialize the MixingBus, as well as Thread and audio inherited
// behaviours.
MixingBus::MixingBus(ActivityManager &am)
    : activityManager(am), playCursor(0), Thread("Background Thread"), totalFrameLength(0), numChannels(2),
      isPlaying(false), uiState(am.getAppState().getUiState()), forwardFFT(FREQVIEW_SAMPLE_FFT_ORDER)
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

    // set master limiter parameters
    masterLimiter.setThreshold(-DSP_DEFAULT_MASTER_LIMITER_HEADROOM_DB);
    masterLimiter.setRelease(DSP_DEFAULT_MASTER_LIMITER_RELEASE_MS);

    // allocate bitmask for tracks
    nearTracksBitmask = new int64_t[SAMPLE_BITMASK_SIZE];
    backgroundNearTrackBitmask = new int64_t[SAMPLE_BITMASK_SIZE];

    // set the nearby samplePlayer/tracks bitmask to 0
    for (size_t i = 0; i < SAMPLE_BITMASK_SIZE; i++)
    {
        *(nearTracksBitmask + i) = 0;
        *(backgroundNearTrackBitmask + i) = 0;
    }

    lastDrawnCursor = 0;

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
            delete track;
        }
    }

    // delete all buffers
    checkForBuffersToFree();

    // free manually allocated arrays
    delete nearTracksBitmask;
    delete backgroundNearTrackBitmask;
}

bool MixingBus::taskHandler(std::shared_ptr<Task> task)
{
    std::shared_ptr<SampleCreateTask> sc = std::dynamic_pointer_cast<SampleCreateTask>(task);

    if (sc != nullptr && !sc->isCompleted() && !sc->hasFailed())
    {
        addSample(sc);
        return true;
    }

    std::shared_ptr<SampleDeletionTask> sd = std::dynamic_pointer_cast<SampleDeletionTask>(task);

    if (sd != nullptr && !sd->isCompleted() && !sd->hasFailed())
    {
        deleteTrack(sd->id);
        return true;
    }

    return false;
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

void MixingBus::addSample(std::shared_ptr<SampleCreateTask> import)
{
    // swap a file to load variable to avoid blocking event thread for disk i/o
    {
        // get lock for scoped block
        const juce::ScopedLock lock(pathMutex);
        importTaskQueue.push_back(import);
    }
    // notify the thread so it's triggered
    notify();
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

    // reset the limiter internal states
    masterLimiter.reset();
    // prepare to play with these settings
    masterLimiter.prepare(currentAudioSpec);

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

    // reset the limiter internal states
    masterLimiter.reset();

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
        // get a pointer to a new processed input buffer from first source
        // we will append into this one to mix tracks together
        if (tracks.getUnchecked(0) != nullptr)
        {
            tracks.getUnchecked(0)->getNextAudioBlock(bufferToFill);
        }
        else
        {
            bufferToFill.clearActiveBufferRegion();
        }

        if (tracks.size() > 1)
        {
            // initialize buffer
            audioThreadBuffer.setSize(juce::jmax(1, bufferToFill.buffer->getNumChannels()),
                                      bufferToFill.buffer->getNumSamples());

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
                }
            }
        }

        // create context to apply dsp effects
        juce::dsp::AudioBlock<float> audioBlockRef(*bufferToFill.buffer, bufferToFill.startSample);
        juce::dsp::ProcessContextReplacing<float> context(audioBlockRef);

        // apply master gain
        masterGain.process(context);

        // apply limiter
        masterLimiter.process(context);
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
        // wake up the background thread if we need to do any redrawing
        if (abs(playCursor - lastDrawnCursor) > FREQVIEW_MIN_REDRAW_DISTANCE_FRAMES)
        {
            notify();
        }
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
        // check for files to import
        checkForFileToImport();
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

void MixingBus::checkForFileToImport()
{

    std::vector<std::shared_ptr<SampleCreateTask>> localTaskQueue;
    // let's preallocate here to avoid doing it on the message thread
    localTaskQueue.reserve(TASK_QUEUE_RESERVED_SIZE);

    {
        const juce::ScopedLock lock(pathMutex);
        importTaskQueue.swap(localTaskQueue);
    }

    // if the path is non empty (meaning we're awaiting importing)
    for (size_t taskIndex = 0; taskIndex < localTaskQueue.size(); taskIndex++)
    {

        if (localTaskQueue[taskIndex]->isDuplication())
        {
            duplicateTrack(localTaskQueue[taskIndex]);
        }
        else
        {
            importNewFile(localTaskQueue[taskIndex]);
        }
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
            SamplePlayer *newSample = new SamplePlayer(desiredPosition);
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
                delete newSample;
                task->setFailed(true);
                return;
            }

            // get a scoped lock for the buffer array
            int newTrackIndex;
            {
                const juce::ScopedLock lock(mixbusMutex);

                // add new SamplePlayer to the tracks list (positionable audio
                // sources)
                tracks.add(newSample);
                newTrackIndex = tracks.size() - 1;
                // add the buffer the array of audio buffers
                buffers.add(newBuffer);
            }

            task->setAllocatedIndex(newTrackIndex);

            task->setCompleted(true);
            std::shared_ptr<SampleDisplayTask> displayTask = std::make_shared<SampleDisplayTask>(newSample, task);
            activityManager.broadcastNestedTaskNow(displayTask);

            std::shared_ptr<ImportFileCountTask> fcTask = std::make_shared<ImportFileCountTask>(pathToOpen);
            activityManager.broadcastNestedTaskNow(fcTask);

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

    // tell all samplePlayers to update positions
    for (size_t i = 0; i < tracks.size(); i++)
    {
        if (tracks.getUnchecked(i) != nullptr)
        {
            // tell the track to reconsider its position
            tracks.getUnchecked(i)->setNextReadPosition(nextReadPosition);
        }
    }

    // we need to repaint the track view !
    trackRepaintCallback();
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

size_t MixingBus::getNumTracks() const
{
    return tracks.size();
}

SamplePlayer *MixingBus::getTrack(int index) const
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

void MixingBus::deleteTrack(int index)
{
    if (index < 0 || index >= tracks.size() || tracks[index] == nullptr)
    {
        return;
    }

    SamplePlayer *track = tracks[index];
    tracks.set(index, nullptr);
    delete track;

    std::shared_ptr<SampleDeletionDisplayTask> displayTask = std::make_shared<SampleDeletionDisplayTask>(index);
    activityManager.broadcastNestedTaskNow(displayTask);
}

void MixingBus::duplicateTrack(std::shared_ptr<SampleCreateTask> task)
{
    // Note: this runs from the background thread

    SamplePlayer *newSample = nullptr;
    int newTrackIndex = -1;

    if (task->getDuplicationType() == DUPLICATION_TYPE_COPY_AT_POSITION)
    {
        newSample = tracks[task->getDuplicateTargetId()]->createDuplicate(task->getPosition());
    }
    else if (task->getDuplicationType() == DUPLICATION_TYPE_SPLIT_AT_FREQUENCY)
    {
        newSample = tracks[task->getDuplicateTargetId()]->splitAtFrequency(task->getSplitFrequency());
    }
    else if (task->getDuplicationType() == DUPLICATION_TYPE_SPLIT_AT_POSITION)
    {
        newSample = tracks[task->getDuplicateTargetId()]->splitAtPosition((int)task->getPosition());
    }

    if (newSample == nullptr)
    {
        task->setFailed(true);
        return;
    }

    // get a scoped lock for the buffer array
    {
        const juce::ScopedLock lock(mixbusMutex);

        // add new SamplePlayer to the tracks list (positionable audio
        // sources)
        tracks.add(newSample);
        newTrackIndex = tracks.size() - 1;
    }

    task->setAllocatedIndex(newTrackIndex);

    task->setCompleted(true);
    std::shared_ptr<SampleDisplayTask> displayTask = std::make_shared<SampleDisplayTask>(newSample, task);
    activityManager.broadcastNestedTaskNow(displayTask);

    trackRepaintCallback();
}