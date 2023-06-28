#include "SampleInputs.h"

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
        auto task = std::make_shared<SampleFadeChange>(numericInputId);
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
    if (numericInputId < 0)
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
                setValue(float(updateTask->currentFadeInFrameLen*1000)/float(AUDIO_FRAMERATE));
            }
        }
        else
        {
            if (!updateTask->onlyFadeIn)
            {
                setValue(float(updateTask->currentFadeOutFrameLen*1000)/float(AUDIO_FRAMERATE));
            }
        }
        // we won't prevent event from being broadcasted further to allow for multiple inputs  to exist
        return false;
    }

    return false;
}

void SampleFadeInput::emitFinalDragTask()
{
    // emit the final task (already completed) so that we record it for reversion
    std::shared_ptr<NumericInputUpdateTask> task =
        std::make_shared<SampleFadeChange>(sampleId, getValue(), 0, getInitialDragValue());
    if (isFadeIn)
    {
        task->onlyFadeIn = true;
    }
    else
    {
        task->onlyFadeOut = true;
    }
    getActivityManager()->broadcastTask(task);
}

void SampleFadeInput::emitIntermediateDragTask(float newValue)
{
    std::shared_ptr<NumericInputUpdateTask> task = std::make_shared<NumericInputUpdateTask>(numericInputId, newValue);
    if (isFadeIn)
    {
        task->onlyFadeIn = true;
    }
    else
    {
        task->onlyFadeOut = true;
    }
    getActivityManager()->broadcastTask(task);
}