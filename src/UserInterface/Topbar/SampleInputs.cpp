#include "SampleInputs.h"
#include <memory>

#define MAX_DB_CHANGE 48.0f
#define DB_CHANGE_STEP 0.2f

SampleFadeInput::SampleFadeInput(bool fadeIn)
    : NumericInput(true, 0, SAMPLEPLAYER_MAX_FADE_MS, 1), isFadeIn(fadeIn), iteratingOverSelection(false)
{
}

void SampleFadeInput::setSampleIds(std::set<size_t> &ids)
{
    if (iteratingOverSelection)
    {
        std::cerr << "Wew ! Selection was updated while iterating over input samples!! This is not cool and should be "
                     "fixed by the clumsy developer!"
                  << std::endl;
    }

    bool newSelectionIsSubset = std::includes(sampleIds.begin(), sampleIds.end(), ids.begin(), ids.end());

    sampleIds = ids;
    displayedSampleId = -1;
    if (sampleIds.size() > 0)
    {
        displayedSampleId = *sampleIds.begin();
    }

    // it's just usueless and slow to update map of initial values
    // when the new selection is a subset. It creates super slow
    // group deletion.
    if (!newSelectionIsSubset)
    {
        initialValues.clear();
        currentValues.clear();
        fetchValueIfPossible();
    }
}

void SampleFadeInput::fetchValueIfPossible()
{
    if (getActivityManager() != nullptr && sampleIds.size() > 0)
    {
        // a copy of the selected set for more safety
        std::set<size_t> actualSelectedIds = sampleIds;
        auto it = actualSelectedIds.begin();
        iteratingOverSelection = true;
        for (it = actualSelectedIds.begin(); it != actualSelectedIds.end(); it++)
        {
            // emit a task that gets the initial value
            auto task = std::make_shared<SampleFadeChange>(*it);
            getActivityManager()->broadcastTask(task);
        }
        iteratingOverSelection = false;
    }
    if (getActivityManager() != nullptr)
    {
        setValue(0);
    }
}

bool SampleFadeInput::taskHandler(std::shared_ptr<Task> task)
{
    // no need to parse tasks if we have no assigned id
    if (sampleIds.size() == 0)
    {
        return false;
    }

    // we are interested in completed SampleFadeChange tasks
    auto updateTask = std::dynamic_pointer_cast<SampleFadeChange>(task);
    if (updateTask != nullptr && updateTask->isCompleted() && !updateTask->hasFailed() &&
        sampleIds.find(updateTask->sampleId) != sampleIds.end())
    {
        if (isFadeIn)
        {
            if (!updateTask->onlyFadeOut)
            {
                if (updateTask->sampleId == displayedSampleId)
                {
                    setValue(float(updateTask->currentFadeInFrameLen * 1000) / float(AUDIO_FRAMERATE));
                }
                currentValues[updateTask->sampleId] = updateTask->currentFadeInFrameLen;
            }
        }
        else
        {
            if (!updateTask->onlyFadeIn)
            {
                if (updateTask->sampleId == displayedSampleId)
                {
                    setValue(float(updateTask->currentFadeOutFrameLen * 1000) / float(AUDIO_FRAMERATE));
                }
                currentValues[updateTask->sampleId] = updateTask->currentFadeOutFrameLen;
            }
        }
        // we won't prevent event from being broadcasted further to allow for multiple inputs  to exist
        return false;
    }

    // if we have a completed task for our selected sample for a resize, request an update
    auto sampleResizeTask = std::dynamic_pointer_cast<SampleTimeCropTask>(task);
    if (sampleResizeTask != nullptr && sampleResizeTask->isCompleted() && !sampleResizeTask->hasFailed() &&
        sampleIds.find(sampleResizeTask->id) != sampleIds.end())
    {
        // Note that we could very well use the finalFadeInFrameLength and its fade out counterpart saved in the task to
        // update here, as we will not emit a sample resize task without setting them. Nervertheless it feels much safer
        // to protect from a corrupted sampleResizeTask and request the actual values to be broadcasted from the
        // MixingBus class.
        auto sampleFadeUpdate = std::make_shared<SampleFadeChange>(sampleResizeTask->id);
        getActivityManager()->broadcastNestedTaskNow(sampleFadeUpdate);
        return false;
    }

    return false;
}

void SampleFadeInput::emitFinalDragTask()
{
    // the pile of tasks to broadcast
    std::vector<std::shared_ptr<SampleFadeChange>> tasks;

    // true if we need to abort due to missing intiial or current value
    // for selection
    bool missingValue = false;

    auto selectedSamplesCopy = sampleIds;
    auto it = selectedSamplesCopy.begin();
    iteratingOverSelection = true;

    int taskGroupId = Task::getNewTaskGroupIndex();

    for (it = selectedSamplesCopy.begin(); it != selectedSamplesCopy.end(); it++)
    {
        // emit the final tasks (already completed) so that we record to be able to revert
        auto task = std::make_shared<SampleFadeChange>(*it, 0, 0, 0, 0);
        task->setTaskGroupIndex(taskGroupId);

        // if we're missing initial or final value, abort
        if (initialValues.find(*it) == initialValues.end() || currentValues.find(*it) == currentValues.end())
        {
            missingValue = true;
            break;
        }

        if (isFadeIn)
        {
            task->onlyFadeIn = true;
            task->previousFadeInFrameLen = initialValues[*it];
            task->currentFadeInFrameLen = currentValues[*it];
        }
        else
        {
            task->onlyFadeOut = true;
            task->previousFadeOutFrameLen = initialValues[*it];
            task->currentFadeOutFrameLen = currentValues[*it];
        }

        // save task to be posted later when we know we're not missing any sample id value
        tasks.push_back(task);
    }

    // abort if we're missing stuff
    if (missingValue)
    {
        std::cerr << "Severe problem: missing multisample input values, aborted updated!!!" << std::endl;
        return;
    }

    // broadcast final task
    for (int i = 0; i < tasks.size(); i++)
    {
        getActivityManager()->broadcastTask(tasks[i]);
    }

    iteratingOverSelection = false;
}

void SampleFadeInput::emitIntermediateDragTask(float newValue)
{
    // list of tasks to post
    std::vector<std::shared_ptr<SampleFadeChange>> tasks;

    // true if we need to abort due to missing intiial or current value
    // for selection
    bool missingValue = false;

    auto selectedSamplesCopy = sampleIds;
    auto it = selectedSamplesCopy.begin();
    iteratingOverSelection = true;

    for (it = selectedSamplesCopy.begin(); it != selectedSamplesCopy.end(); it++)
    {
        std::shared_ptr<SampleFadeChange> task;

        // if we're missing initial or final value, abort
        if (initialValues.find(*it) == initialValues.end() || currentValues.find(*it) == currentValues.end())
        {
            missingValue = true;
            break;
        }

        if (isFadeIn)
        {
            task = std::make_shared<SampleFadeChange>(*it, newValue * (float(AUDIO_FRAMERATE) / 1000.0), 0);
            task->onlyFadeIn = true;
        }
        else
        {
            task = std::make_shared<SampleFadeChange>(*it, 0, newValue * (float(AUDIO_FRAMERATE) / 1000.0));
            task->onlyFadeOut = true;
        }
        tasks.push_back(task);
    }

    // abort if we're missing stuff
    if (missingValue)
    {
        std::cerr << "Severe problem: missing multisample input values, aborted intermediate updated!!!" << std::endl;
        return;
    }

    for (int i = 0; i < tasks.size(); i++)
    {
        getActivityManager()->broadcastTask(tasks[i]);
    }

    iteratingOverSelection = false;
}

void SampleFadeInput::startDragging()
{
    initialValues = currentValues;
}

/////////////////////////////////////////////////////////////////////////

SampleGainInput::SampleGainInput()
    : NumericInput(false, -MAX_DB_CHANGE, MAX_DB_CHANGE, DB_CHANGE_STEP), iteratingOverSelection(false)
{
}

void SampleGainInput::setSampleIds(std::set<size_t> &ids)
{
    if (iteratingOverSelection)
    {
        std::cerr << "Wew ! Selection was updated while iterating over input samples!! This is not cool and should be "
                     "fixed by the clumsy developer! (gain input version)"
                  << std::endl;
    }

    bool newSelectionIsSubset = std::includes(sampleIds.begin(), sampleIds.end(), ids.begin(), ids.end());

    sampleIds = ids;
    displayedSampleId = -1;
    if (sampleIds.size() > 0)
    {
        displayedSampleId = *sampleIds.begin();
    }

    // it's just usueless and slow to update map of initial values
    // when the new selection is a subset. It creates super slow
    // group deletion.
    if (!newSelectionIsSubset)
    {
        initialValues.clear();
        currentValues.clear();
        fetchValueIfPossible();
    }
}

void SampleGainInput::fetchValueIfPossible()
{
    if (getActivityManager() != nullptr && sampleIds.size() > 0)
    {
        // a copy of the selected set for more safety
        std::set<size_t> actualSelectedIds = sampleIds;
        auto it = actualSelectedIds.begin();
        iteratingOverSelection = true;
        for (it = actualSelectedIds.begin(); it != actualSelectedIds.end(); it++)
        {
            // emit a task that gets the initial value
            auto task = std::make_shared<SampleGainChange>(*it);
            getActivityManager()->broadcastTask(task);
        }
        iteratingOverSelection = false;
    }
    if (getActivityManager() != nullptr)
    {
        setValue(0);
    }
}

bool SampleGainInput::taskHandler(std::shared_ptr<Task> task)
{
    if (sampleIds.size() == 0)
    {
        return false;
    }

    auto updateTask = std::dynamic_pointer_cast<SampleGainChange>(task);
    if (updateTask != nullptr && updateTask->isCompleted() && !updateTask->hasFailed() &&
        sampleIds.find(updateTask->sampleId) != sampleIds.end())
    {
        if (updateTask->sampleId == displayedSampleId)
        {
            setValue(updateTask->currentDbGain);
        }
        currentValues[updateTask->sampleId] = updateTask->currentDbGain;

        // we won't prevent event from being broadcasted further to allow for multiple inputs  to exist
        return false;
    }

    return false;
}

void SampleGainInput::emitFinalDragTask()
{
    // the pile of tasks to broadcast
    std::vector<std::shared_ptr<SampleGainChange>> tasks;

    // true if we need to abort due to missing intiial or current value
    // for selection
    bool missingValue = false;

    auto selectedSamplesCopy = sampleIds;
    auto it = selectedSamplesCopy.begin();
    iteratingOverSelection = true;

    int taskGroupId = Task::getNewTaskGroupIndex();

    for (it = selectedSamplesCopy.begin(); it != selectedSamplesCopy.end(); it++)
    {
        // if we're missing initial or final value, abort
        if (initialValues.find(*it) == initialValues.end() || currentValues.find(*it) == currentValues.end())
        {
            missingValue = true;
            break;
        }

        // emit the final tasks (already completed) so that we record to be able to revert
        auto task = std::make_shared<SampleGainChange>(*it, initialValues[*it], currentValues[*it]);
        task->setTaskGroupIndex(taskGroupId);

        // save task to be posted later when we know we're not missing any sample id value
        tasks.push_back(task);
    }

    // abort if we're missing stuff
    if (missingValue)
    {
        std::cerr << "Severe problem: missing multisample input values, aborted updated!!!" << std::endl;
        return;
    }

    // broadcast final task
    for (int i = 0; i < tasks.size(); i++)
    {
        getActivityManager()->broadcastTask(tasks[i]);
    }

    iteratingOverSelection = false;
}

void SampleGainInput::emitIntermediateDragTask(float newValue)
{
    // list of tasks to post
    std::vector<std::shared_ptr<SampleGainChange>> tasks;

    // true if we need to abort due to missing intiial or current value
    // for selection
    bool missingValue = false;

    auto selectedSamplesCopy = sampleIds;
    auto it = selectedSamplesCopy.begin();
    iteratingOverSelection = true;

    for (it = selectedSamplesCopy.begin(); it != selectedSamplesCopy.end(); it++)
    {
        std::shared_ptr<SampleGainChange> task;

        // if we're missing initial or final value, abort
        if (initialValues.find(*it) == initialValues.end() || currentValues.find(*it) == currentValues.end())
        {
            missingValue = true;
            break;
        }

        task = std::make_shared<SampleGainChange>(*it, newValue);
        tasks.push_back(task);
    }

    // abort if we're missing stuff
    if (missingValue)
    {
        std::cerr << "Severe problem: missing multisample input values (gain version), aborted intermediate updated!!!"
                  << std::endl;
        return;
    }

    for (int i = 0; i < tasks.size(); i++)
    {
        getActivityManager()->broadcastTask(tasks[i]);
    }

    iteratingOverSelection = false;
}

void SampleGainInput::startDragging()
{
    initialValues = currentValues;
}
