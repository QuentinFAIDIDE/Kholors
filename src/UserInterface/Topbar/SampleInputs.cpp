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
        auto task = std::make_shared<AAAAAA>(numericInputId);
        getActivityManager()->broadcastTask(task);
    }
    if (sampleId < 0)
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

    // we are interested in completed NumericInputUpdateTask
    std::shared_ptr<NumericInputUpdateTask> updateTask = std::dynamic_pointer_cast<NumericInputUpdateTask>(task);
    if (updateTask != nullptr && updateTask->isCompleted() && !updateTask->hasFailed() &&
        updateTask->numericalInputId == numericInputId)
    {
        setValue(updateTask->newValue);
        // we won't prevent event from being broadcasted further to allow for multiple inputs
        // to work on the same numeric id
        return false;
    }

    return false;
}

void SampleFadeInput::emitFinalDragTask()
{
    // emit the final task (already completed) so that we record it for reversion
    std::shared_ptr<NumericInputUpdateTask> task =
        std::make_shared<NumericInputUpdateTask>(numericInputId, getValue(), getInitialDragValue());
    getActivityManager()->broadcastTask(task);
}

void SampleFadeInput::emitIntermediateDragTask(float newValue)
{
    std::shared_ptr<NumericInputUpdateTask> task = std::make_shared<NumericInputUpdateTask>(numericInputId, newValue);
    getActivityManager()->broadcastTask(task);
}