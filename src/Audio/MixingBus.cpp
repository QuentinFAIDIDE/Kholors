#include "MixingBus.h"
#include "DataSource.h"
#include "MixbusDataSource.h"
#include "SamplePlayer.h"
#include "UnitConverter.h"

#include <cstdint>
#include <iostream>
#include <memory>

#include <stdexcept>

// initialize the MixingBus, as well as Thread and audio inherited
// behaviours.
MixingBus::MixingBus(ActivityManager &am)
    : Thread("Mixbus Loader Thread"), activityManager(am), numChannels(2), uiState(am.getAppState().getUiState())
{

    // create instance of mixbusDataSource
    mixbusDataSource = std::make_shared<MixbusDataSource>();

    reset();

    // start the background thread that makes malloc/frees
    startThread();
}

void MixingBus::reset()
{
    playCursor = 0;
    isPlaying = false;
    loopingToggledOn = false;
    loopSectionStartFrame = 22050;
    loopSectionEndFrame = 44100;

    mixbusDataSource->setPosition(playCursor);

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

    samplePlayers.clear();
}

MixingBus::~MixingBus()
{
    // stop thread with a 4sec timeout to kill it
    stopThread(5000);

    // delete all tracks
    for (size_t i = 0; i < (size_t)samplePlayers.size(); i++)
    {
        auto track = samplePlayers.getUnchecked(i);
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

std::string MixingBus::marshal()
{
    const juce::ScopedLock lock(mixbusMutex);

    json samplePlayersJSON;
    for (int i = 0; i < samplePlayers.size(); i++)
    {
        if (samplePlayers[i] != nullptr)
        {
            samplePlayersJSON.push_back(samplePlayers[i]->toJSON());
        }
        else
        {
            samplePlayersJSON.push_back(nullptr);
        }
    }

    json output = {{"loop_start_frame", loopSectionStartFrame},
                   {"loop_end_frame", loopSectionEndFrame},
                   {"loop_is_on", loopingToggledOn},
                   {"play_cursor_position_frame", playCursor},
                   {"master_gain", masterGain.getGainLinear()},
                   {"sample_players", samplePlayersJSON}};

    return output.dump(JSON_STATE_SAVING_INDENTATION);
}

void MixingBus::unmarshal(std::string &s)
{
    const juce::ScopedLock lock(mixbusMutex);

    isPlaying = false;

    json input = json::parse(s);

    auto loopStartEntry = input.at("loop_start_frame");
    loopSectionStartFrame = loopStartEntry.template get<int64_t>();

    auto loopEndEntry = input.at("loop_end_frame");
    loopSectionEndFrame = loopEndEntry.template get<int64_t>();

    auto loopIsOnEntry = input.at("loop_is_on");
    loopingToggledOn = loopIsOnEntry.template get<bool>();

    auto playCursorEntry = input.at("play_cursor_position_frame");
    playCursor = playCursorEntry.template get<int>();

    auto masterGainEntry = input.at("master_gain");
    float masterGainLinear = masterGainEntry.template get<float>();
    masterGain.setGainLinear(masterGainLinear);

    // we should request a broadcast of the loop section status (toggle and position)
    auto loopResetTask = std::make_shared<LoopMovingTask>();
    activityManager.broadcastNestedTaskNow(loopResetTask);

    auto loopToggle = std::make_shared<LoopToggleTask>();
    activityManager.broadcastNestedTaskNow(loopToggle);

    auto samplePlayersEntry = input.at("sample_players");

    if (!samplePlayersEntry.is_array())
    {
        throw std::runtime_error("Receive a json sample player list that is not an array!");
    }

    samplePlayers.clear();
    samplePlayers.ensureStorageAllocated(samplePlayersEntry.size());
    for (size_t i = 0; i < samplePlayersEntry.size(); i++)
    {
        // we leave the previously existing gaps so that the sample ids stay the same
        if (samplePlayersEntry[i].is_null())
        {
            samplePlayers.add(nullptr);

            // this is necessary to increment index of openGL object as well
            // they must match exactly with samplePlayers lists
            std::shared_ptr<SampleCreateTask> completedCreateTask =
                std::make_shared<SampleCreateTask>("", samplePlayers.size() - 1);
            completedCreateTask->setCompleted(true);
            completedCreateTask->reuseNewId = false;
            completedCreateTask->setAllocatedIndex(samplePlayers.size() - 1);

            std::shared_ptr<SampleDisplayTask> displayTask =
                std::make_shared<SampleDisplayTask>(nullptr, completedCreateTask);
            activityManager.broadcastNestedTaskNow(displayTask);
        }
        else
        {
            std::string filePath;
            samplePlayersEntry[i].at("file_path").get_to(filePath);

            std::shared_ptr<SampleCreateTask> task = std::make_shared<SampleCreateTask>(filePath, 0);
            activityManager.broadcastNestedTaskNow(task);

            if (task->hasFailed())
            {
                throw std::runtime_error("A sample failed to be loaded.");
            }

            samplePlayers[i]->setupFromJSON(samplePlayersEntry[i]);

            auto updateViewTask = std::make_shared<SampleUpdateTask>(i, samplePlayers[i]);
            activityManager.broadcastNestedTaskNow(updateViewTask);
            if (updateViewTask->hasFailed())
            {
                throw std::runtime_error("A sample failed to have its view updated.");
            }
        }
    }
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
        if (samplePlayers[moveTask->id] != nullptr)
        {
            samplePlayers[moveTask->id]->move(samplePlayers[moveTask->id]->getEditingPosition() +
                                              moveTask->dragDistance);
            moveTask->setCompleted(true);
            moveTask->setFailed(false);

            // we need to tell arrangement are to update
            std::shared_ptr<SampleUpdateTask> sut =
                std::make_shared<SampleUpdateTask>(moveTask->id, samplePlayers[moveTask->id]);
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
            // If in loop mode, a reset takes you to loop start.
            // If not it goes to the track beginning.
            int appropriateResetPositon = 0;
            if (loopingToggledOn)
            {
                appropriateResetPositon = loopSectionStartFrame;
            }

            if (!isPlaying)
            {
                if (playUpdate->shouldResetPosition)
                {
                    {
                        const juce::ScopedLock lock(mixbusMutex);
                        playCursor = appropriateResetPositon;
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
                    playCursor = appropriateResetPositon;
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
        if (fadeChangeTask->sampleId < 0 || fadeChangeTask->sampleId >= samplePlayers.size() ||
            samplePlayers[fadeChangeTask->sampleId] == nullptr)
        {
            std::cerr << "received a fade change task for a sample id that does not exists: "
                      << fadeChangeTask->sampleId << std::endl;
            return false;
        }

        // broadcast result if it's what we need to do
        if (fadeChangeTask->isBroadcastRequest)
        {
            fadeChangeTask->currentFadeInFrameLen = samplePlayers[fadeChangeTask->sampleId]->getFadeInLength();
            fadeChangeTask->currentFadeOutFrameLen = samplePlayers[fadeChangeTask->sampleId]->getFadeOutLength();
            fadeChangeTask->setCompleted(true);
            activityManager.broadcastNestedTaskNow(fadeChangeTask);
            return true;
        }

        bool hasUpdated = false;
        if (!fadeChangeTask->onlyFadeOut)
        {
            bool fadeInUpdate =
                samplePlayers[fadeChangeTask->sampleId]->setFadeInLength(fadeChangeTask->currentFadeInFrameLen);
            hasUpdated = fadeInUpdate;

            fadeChangeTask->currentFadeInFrameLen = samplePlayers[fadeChangeTask->sampleId]->getFadeInLength();
        }

        if (!fadeChangeTask->onlyFadeIn)
        {
            bool fadeOutUpdate =
                samplePlayers[fadeChangeTask->sampleId]->setFadeOutLength(fadeChangeTask->currentFadeOutFrameLen);
            hasUpdated = hasUpdated || fadeOutUpdate;
            fadeChangeTask->currentFadeOutFrameLen = samplePlayers[fadeChangeTask->sampleId]->getFadeOutLength();
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
        if (gainChangeTask->sampleId < 0 || gainChangeTask->sampleId >= samplePlayers.size() ||
            samplePlayers[gainChangeTask->sampleId] == nullptr)
        {
            return false;
        }

        if (!gainChangeTask->isBroadcastRequest)
        {
            samplePlayers[gainChangeTask->sampleId]->setDbGain(gainChangeTask->currentDbGain);
        }
        gainChangeTask->currentDbGain = samplePlayers[gainChangeTask->sampleId]->getDbGain();

        gainChangeTask->setCompleted(true);
        activityManager.broadcastNestedTaskNow(gainChangeTask);
        return true;
    }

    auto filterRepeatChange = std::dynamic_pointer_cast<SampleFilterRepeatChange>(task);
    if (filterRepeatChange != nullptr && !filterRepeatChange->isCompleted() && !filterRepeatChange->hasFailed())
    {
        if (filterRepeatChange->sampleId < 0 || filterRepeatChange->sampleId >= samplePlayers.size() ||
            samplePlayers[filterRepeatChange->sampleId] == nullptr)
        {
            return false;
        }

        if (filterRepeatChange->isLowPassFilter)
        {
            filterRepeatChange->previousFilterRepeat = samplePlayers[filterRepeatChange->sampleId]->getLowPassRepeat();
        }
        else
        {
            filterRepeatChange->previousFilterRepeat = samplePlayers[filterRepeatChange->sampleId]->getHighPassRepeat();
        }

        if (!filterRepeatChange->isBroadcastRequest)
        {
            if (filterRepeatChange->isLowPassFilter)
            {
                samplePlayers[filterRepeatChange->sampleId]->setLowPassRepeat(filterRepeatChange->newFilterRepeat);
            }
            else
            {
                samplePlayers[filterRepeatChange->sampleId]->setHighPassRepeat(filterRepeatChange->newFilterRepeat);
            }
        }

        if (filterRepeatChange->isLowPassFilter)
        {
            filterRepeatChange->newFilterRepeat = samplePlayers[filterRepeatChange->sampleId]->getLowPassRepeat();
        }
        else
        {
            filterRepeatChange->newFilterRepeat = samplePlayers[filterRepeatChange->sampleId]->getHighPassRepeat();
        }

        filterRepeatChange->setCompleted(true);
        activityManager.broadcastNestedTaskNow(filterRepeatChange);
        return true;
    }

    auto resetTask = std::dynamic_pointer_cast<ResetTask>(task);
    if (resetTask != nullptr)
    {

        // get audio thread lock
        const juce::ScopedLock lock(mixbusMutex);

        reset();
        notify();

        auto loopResetTask = std::make_shared<LoopMovingTask>();
        loopResetTask->isBroadcastRequest = false;
        loopResetTask->currentLoopBeginFrame = loopSectionStartFrame;
        loopResetTask->currentLoopEndFrame = loopSectionEndFrame;
        loopResetTask->setCompleted(true);
        activityManager.broadcastNestedTaskNow(loopResetTask);

        auto loopToggle = std::make_shared<LoopToggleTask>(false);
        activityManager.broadcastNestedTaskNow(loopToggle);

        resetTask->markStepDoneAndCheckCompletion();

        return false;
    }

    auto projectLoadingTask = std::dynamic_pointer_cast<OpenProjectTask>(task);
    if (projectLoadingTask != nullptr && projectLoadingTask->stage == OPEN_PROJECT_STAGE_MIXBUS_SETUP &&
        !projectLoadingTask->hasFailed())
    {
        try
        {
            unmarshal(projectLoadingTask->mixbusConfig);
            projectLoadingTask->setCompleted(true);
            projectLoadingTask->stage = OPEN_PROJECT_STAGE_COMPLETED;
        }
        catch (std::exception &err)
        {
            projectLoadingTask->setFailed(true);
            projectLoadingTask->stage = OPEN_PROJECT_STAGE_FAILED;
            std::cerr << "unable to open project on mixbus side: " << err.what() << std::endl;

            auto notifTask = std::make_shared<NotificationTask>("Unable to open project. See logs for more infos.",
                                                                ERROR_NOTIF_TYPE);
            activityManager.broadcastNestedTaskNow(notifTask);
        }

        return true;
    }

    return false;
}

void MixingBus::cropSample(std::shared_ptr<SampleTimeCropTask> task)
{
    if (samplePlayers[task->id] != nullptr)
    {

        int initialFadeIn = samplePlayers[task->id]->getFadeInLength();
        int initialFadeOut = samplePlayers[task->id]->getFadeOutLength();

        if (task->movingBeginning)
        {
            samplePlayers[task->id]->tryMovingStart(task->dragDistance);
        }
        else
        {
            samplePlayers[task->id]->tryMovingEnd(task->dragDistance);
        }

        // if the fade changed due to resize, broadcast the change
        int finalFadeIn = samplePlayers[task->id]->getFadeInLength();
        int finalFadeOut = samplePlayers[task->id]->getFadeOutLength();

        // record the initial and final fade in and out, because if they were
        // altered by resize, we might need to post a SampleFadeChange
        // task when reverting this resizing task
        task->initialFadeInFrameLen = initialFadeIn;
        task->initialFadeOutFrameLen = initialFadeOut;
        task->finalFadeInFrameLen = finalFadeIn;
        task->finalFadeOutFrameLen = finalFadeOut;

        task->setCompleted(true);
        task->setFailed(false);

        // we need to tell arrangement are to update
        std::shared_ptr<SampleUpdateTask> sut = std::make_shared<SampleUpdateTask>(task->id, samplePlayers[task->id]);
        activityManager.broadcastNestedTaskNow(sut);

        // if the fade length changed, make the ui widgets update!
        if (initialFadeIn != finalFadeIn || initialFadeOut != finalFadeOut)
        {
            auto fadeUpdate =
                std::make_shared<SampleFadeChange>(task->id, initialFadeIn, initialFadeOut, finalFadeIn, finalFadeOut);
            fadeUpdate->preventFromGoingToTaskHistory();
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
    if (samplePlayers[task->id] != nullptr)
    {
        if (task->isLowPass)
        {
            samplePlayers[task->id]->setLowPassFreq(task->finalFrequency);
        }
        else
        {
            samplePlayers[task->id]->setHighPassFreq(task->finalFrequency);
        }

        task->setCompleted(true);
        task->setFailed(false);

        // we need to tell arrangement are to update
        std::shared_ptr<SampleUpdateTask> sut = std::make_shared<SampleUpdateTask>(task->id, samplePlayers[task->id]);
        activityManager.broadcastNestedTaskNow(sut);

        trackRepaintCallback();
    }
    else
    {
        task->setCompleted(true);
        task->setFailed(true);
    }
}

bool MixingBus::filePathsValid(const juce::StringArray &)
{
    // TODO: implement this
    return true;
}

void MixingBus::startPlayback()
{
    if (!isPlaying)
    {
        const juce::ScopedLock lock(mixbusMutex);
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
    for (size_t i = 0; i < (size_t)samplePlayers.size(); i++)
    {
        if (samplePlayers.getUnchecked(i) != nullptr)
        {
            samplePlayers.getUnchecked(i)->prepareToPlay(samplesPerBlockExpected, sampleRate);
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
    for (size_t i = 0; i < (size_t)samplePlayers.size(); i++)
    {
        if (samplePlayers.getUnchecked(i) != nullptr)
        {
            samplePlayers.getUnchecked(i)->releaseResources();
        }
    }

    masterGain.reset();

    // clear output buffer
    audioThreadBuffer.setSize(2, 0);
}

void MixingBus::getNextAudioBlock(const juce::AudioSourceChannelInfo &bufferToFill)
{
    const juce::ScopedLock sl(mixbusMutex);

    int nextPlayPosition = (playCursor + bufferToFill.numSamples);
    bool needToLoop = loopingToggledOn && playCursor <= loopSectionEndFrame && nextPlayPosition > loopSectionEndFrame;

    // if not in loop mode or the end of the loop section is not in range, just proceed to get the full block
    if (!needToLoop)
    {
        getAudioBlock(bufferToFill);
    }
    else
    {
        // we will basically split the work in two AudioSourceChannelInfo

        // we will need to shift the start position of the second AudioSourceChannelInfo
        int numSamplesInLoopBounds = (loopSectionEndFrame - playCursor) + 1;
        int nextBufferStart = bufferToFill.startSample + numSamplesInLoopBounds;
        // as well as reduce its length
        int nextBufferLength = bufferToFill.numSamples - numSamplesInLoopBounds;

        // and we will also need to shrink the first one
        int currentBufferLength = numSamplesInLoopBounds;

        // we execute the first part (untill we read the last sample of the loop)
        juce::AudioSourceChannelInfo loopEndBufferRead(bufferToFill.buffer, bufferToFill.startSample,
                                                       currentBufferLength);
        getAudioBlock(loopEndBufferRead);

        // we reset the position to the start of the loop
        setNextReadPosition(loopSectionStartFrame);

        // If the queried section perfectly fit loop end, next buffer will have size of zero and second step is
        // unecessary
        if (nextBufferLength > 0)
        {
            // and fetch the rest of the buffer
            juce::AudioSourceChannelInfo loopStartBufferRead(bufferToFill.buffer, nextBufferStart, nextBufferLength);
            getAudioBlock(loopStartBufferRead);
        }
    }
}

void MixingBus::getAudioBlock(const juce::AudioSourceChannelInfo &bufferToFill)
{
    // the mixing code here was initially based on the MixerAudioSource one from
    // Juce.

    // Honestly, I feel like it should be rewritten in a more straightforward manner
    // that would always call clearActiveBufferRegion at first and then add the samples audio blocks.
    // It would then have a much simpler structure.
    // It could also benefit from factoring out the various steps and utilities at the end under their own functions.

    // if there is more then one input track and we are playing
    if (samplePlayers.size() > 0 && isPlaying)
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
        if (samplePlayers.getUnchecked(0) != nullptr)
        {
            samplePlayers.getUnchecked(0)->getNextAudioBlock(bufferToFill);

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

        if (samplePlayers.size() > 1)
        {
            // initialize buffer
            audioThreadBuffer.setSize(juce::jmax(1, bufferToFill.buffer->getNumChannels()),
                                      bufferToFill.buffer->getNumSamples(), false, false, true);

            // create a new getNextAudioBlock request that
            // will use our MixingBus buffer to pull
            // block to append to the previous buffer
            juce::AudioSourceChannelInfo copyBufferDest(&audioThreadBuffer, 0, bufferToFill.numSamples);

            // for each input source
            for (size_t i = 1; i < (size_t)samplePlayers.size(); i++)
            {
                if (samplePlayers.getUnchecked(i) == nullptr)
                {
                    continue;
                }
                // get the next audio block in the buffer
                samplePlayers.getUnchecked(i)->getNextAudioBlock(copyBufferDest);
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
        juce::dsp::AudioBlock<float> audioBlockRef(*bufferToFill.buffer, (size_t)bufferToFill.startSample);
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
        wait(3000);
    }
}

void MixingBus::checkForCursorRedraw()
{
    // if we were notified to redraw, do it.
    // NOTE: It might actually be bloody cocking useless, since we check for redraw in the getAudioBlock callbacks
    if (abs(lastDrawnCursor - playCursor) > FREQVIEW_MIN_REDRAW_DISTANCE_FRAMES)
    {
        lastDrawnCursor = playCursor;
        const juce::MessageManagerLock mmLock;
        trackRepaintCallback();
    }
}

void MixingBus::checkForBuffersToFree()
{
    sharedAudioFileBuffers->releaseUnusedBuffers();
}

void MixingBus::importNewFile(std::shared_ptr<SampleCreateTask> task)
{
    // get the necessary task parameters
    std::string pathToOpen = task->getFilePath();
    int64_t desiredPosition = task->getPosition();

    // request the audio buffer to be loaded
    try
    {
        auto fileAudioBuffer = sharedAudioFileBuffers->loadSample(pathToOpen);

        // create a new sample player
        std::shared_ptr<SamplePlayer> newSample = std::make_shared<SamplePlayer>(desiredPosition);

        // assign buffer to the new SamplePlayer
        newSample->setBuffer(fileAudioBuffer);

        // get a scoped lock for the buffer array
        int newTrackIndex;
        {
            const juce::ScopedLock lock(mixbusMutex);

            // add new SamplePlayer to the tracks list (positionable audio
            // sources)
            if (task->reuseNewId)
            {
                samplePlayers.set(task->getAllocatedIndex(), newSample);
                newTrackIndex = task->getAllocatedIndex();
            }
            else
            {
                samplePlayers.add(newSample);
                newTrackIndex = samplePlayers.size() - 1;
            }

            samplePlayers[newTrackIndex]->setNextReadPosition(playCursor);
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
    catch (std::runtime_error &err)
    {
        std::cerr << "File " << pathToOpen << " failed to be loaded: " << err.what() << std::endl;

        auto notif = std::make_shared<NotificationTask>(std::string() + "Unable to import file: " + err.what(),
                                                        ERROR_NOTIF_TYPE);

        activityManager.broadcastTask(notif); /* note that if called from within a task broadcastTask will wait after
                                                 execution of current task to perform the new one */

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
    for (size_t i = 0; i < (size_t)samplePlayers.size(); i++)
    {
        if (samplePlayers.getUnchecked(i) != nullptr)
        {
            // tell the track to reconsider its position
            samplePlayers.getUnchecked(i)->setNextReadPosition(nextReadPosition);
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
    return 0;
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
    return (size_t)samplePlayers.size();
}

std::shared_ptr<SamplePlayer> MixingBus::getTrack(int index) const
{
    // NOTE: We never delete SamplePlayers in the tracks array or insert mid-array
    // to prevent using a lock when reading.

    // get pointer to the value at index
    return samplePlayers.getUnchecked(index);
}

void MixingBus::setTrackRepaintCallback(std::function<void()> f)
{
    trackRepaintCallback = f;
}

void MixingBus::deleteSample(std::shared_ptr<SampleDeletionTask> deletionTask)
{
    if (deletionTask->id < 0 || deletionTask->id >= samplePlayers.size() || samplePlayers[deletionTask->id] == nullptr)
    {
        deletionTask->setCompleted(true);
        deletionTask->setFailed(true);
        return;
    }

    // we save a reference to this sample in the tasks list to restore it if
    // users wants to.
    deletionTask->deletedSample = samplePlayers[deletionTask->id];

    // clear this sample
    {
        const juce::ScopedLock lock(mixbusMutex);
        samplePlayers.set(deletionTask->id, std::shared_ptr<SamplePlayer>(nullptr));
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
    if (task->id < 0 || task->id >= samplePlayers.size() || samplePlayers[task->id] != nullptr ||
        task->sampleToRestore == nullptr)
    {
        task->setCompleted(true);
        task->setFailed(true);
        return;
    }

    samplePlayers.set(task->id, task->sampleToRestore);

    std::shared_ptr<SampleRestoreDisplayTask> displayTask =
        std::make_shared<SampleRestoreDisplayTask>(task->id, task->sampleToRestore);
    activityManager.broadcastNestedTaskNow(displayTask);

    task->setCompleted(true);
    task->setFailed(false);
}

void MixingBus::duplicateTrack(std::shared_ptr<SampleCreateTask> task)
{
    // Note: this runs from the background thread

    if (task->getDuplicateTargetId() < 0 || task->getDuplicateTargetId() >= samplePlayers.size())
    {

        task->setFailed(true);
        return;
    }

    // get a reference to the buffer to duplicate
    std::shared_ptr<SamplePlayer> targetToDuplicate = samplePlayers[task->getDuplicateTargetId()];

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
            samplePlayers.set(task->getAllocatedIndex(), newSample);
        }
        else
        {
            // add new SamplePlayer to the tracks list (positionable audio
            // sources)
            samplePlayers.add(newSample);
            newTrackIndex = samplePlayers.size() - 1;
        }
        samplePlayers[newTrackIndex]->setNextReadPosition(playCursor);
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