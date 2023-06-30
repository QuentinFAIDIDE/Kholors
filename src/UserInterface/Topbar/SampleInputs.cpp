#include "SampleInputs.h"
#include <memory>

#define MAX_DB_CHANGE 12.0f
#define DB_CHANGE_STEP 0.2f

SampleFadeInput::SampleFadeInput(bool fadeIn)
    : NumericInput(true, 0, SAMPLEPLAYER_MAX_FADE_MS, 1), sampleId(-1), isFadeIn(fadeIn)
{
}

void SampleFadeInput::setSampleId(int id)
{
    sampleId = id;
    fetchValueIfPossible();
}

void SampleFadeInput::fetchValueIfPossible()
{
    if (getActivityManager() != nullptr && sampleId >= 0)
    {
        // emit a task that gets the initial value
        auto task = std::make_shared<SampleFadeChange>(sampleId);
        getActivityManager()->broadcastTask(task);
    }
    if (getActivityManager() != nullptr && sampleId < 0)
    {
        setValue(0);
    }
}

bool SampleFadeInput::taskHandler(std::shared_ptr<Task> task)
{
    // no need to parse tasks if we have no assigned id
    if (sampleId < 0)
    {
        return false;
    }

    // we are interested in completed SampleFadeChange tasks
    auto updateTask = std::dynamic_pointer_cast<SampleFadeChange>(task);
    if (updateTask != nullptr && updateTask->isCompleted() && !updateTask->hasFailed() &&
        updateTask->sampleId == sampleId)
    {
        if (isFadeIn)
        {
            if (!updateTask->onlyFadeOut)
            {
                setValue(float(updateTask->currentFadeInFrameLen * 1000) / float(AUDIO_FRAMERATE));
            }
        }
        else
        {
            if (!updateTask->onlyFadeIn)
            {
                setValue(float(updateTask->currentFadeOutFrameLen * 1000) / float(AUDIO_FRAMERATE));
            }
        }
        // we won't prevent event from being broadcasted further to allow for multiple inputs  to exist
        return false;
    }

    return false;
}

void SampleFadeInput::emitFinalDragTask()
{
    // emit the final task (already completed) so that we record to be able to revert
    auto task = std::make_shared<SampleFadeChange>(sampleId, 0, 0, 0, 0);
    if (isFadeIn)
    {
        task->onlyFadeIn = true;
        task->previousFadeInFrameLen = getInitialDragValue() * (float(AUDIO_FRAMERATE) / 1000.0);
        task->currentFadeInFrameLen = getValue() * (float(AUDIO_FRAMERATE) / 1000.0);
    }
    else
    {
        task->onlyFadeOut = true;
        task->previousFadeOutFrameLen = getInitialDragValue() * (float(AUDIO_FRAMERATE) / 1000.0);
        task->currentFadeOutFrameLen = getValue() * (float(AUDIO_FRAMERATE) / 1000.0);
    }
    getActivityManager()->broadcastTask(task);
}

void SampleFadeInput::emitIntermediateDragTask(float newValue)
{
    std::shared_ptr<SampleFadeChange> task;

    if (isFadeIn)
    {
        task = std::make_shared<SampleFadeChange>(sampleId, newValue * (float(AUDIO_FRAMERATE) / 1000.0), 0);
        task->onlyFadeIn = true;
    }
    else
    {
        task = std::make_shared<SampleFadeChange>(sampleId, 0, newValue * (float(AUDIO_FRAMERATE) / 1000.0));
        task->onlyFadeOut = true;
    }
    getActivityManager()->broadcastTask(task);
}

/////////////////////////////////////////////////////////////////////////

SampleGainInput::SampleGainInput() : NumericInput(false, -MAX_DB_CHANGE, MAX_DB_CHANGE, DB_CHANGE_STEP), sampleId(-1)
{
}

void SampleGainInput::setSampleId(int id)
{
    sampleId = id;
    fetchValueIfPossible();
}

void SampleGainInput::fetchValueIfPossible()
{
    if (getActivityManager() != nullptr && sampleId >= 0)
    {
        // emit a task that gets the initial value
        auto task = std::make_shared<SampleGainChange>(sampleId);
        getActivityManager()->broadcastTask(task);
    }
    if (getActivityManager() != nullptr && sampleId < 0)
    {
        setValue(0);
    }
}

bool SampleGainInput::taskHandler(std::shared_ptr<Task> task)
{
    // no need to parse tasks if we have no assigned id
    if (sampleId < 0)
    {
        return false;
    }

    // we are interested in completed SampleGainChange tasks
    auto updateTask = std::dynamic_pointer_cast<SampleGainChange>(task);
    if (updateTask != nullptr && updateTask->isCompleted() && !updateTask->hasFailed() &&
        updateTask->sampleId == sampleId)
    {
        setValue(updateTask->currentDbGain);
        // we won't prevent event from being broadcasted further to allow for multiple inputs  to exist
        return false;
    }

    return false;
}

void SampleGainInput::emitFinalDragTask()
{
    // emit the final task (already completed) so that we record to be able to revert
    auto task = std::make_shared<SampleGainChange>(sampleId, getInitialDragValue(), getValue());
    getActivityManager()->broadcastTask(task);
}

void SampleGainInput::emitIntermediateDragTask(float newValue)
{
    auto task = std::make_shared<SampleGainChange>(sampleId, newValue);
    getActivityManager()->broadcastTask(task);
}