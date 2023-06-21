#include "Task.h"
#include "TaxonomyManager.h"
#include <memory>

Task::Task()
{
    completed = false;
    failed = false;

    // default to record tasks in history.
    // if you don't want to, inherit SilentTask
    recordableInHistory = true;

    isPartOfReversion = false;
}

Task::~Task()
{
}

std::string Task::marshal()
{
    json taskj = {
        {"object", "task"},
        {"task", "generic_task"},
        {"is_completed", completed},
        {"failed", failed},
        {"recordable_in_history", recordableInHistory},
        {"reversed", false},
    };
    return taskj.dump();
}

Marshalable *Task::unmarshal(std::string &)
{
    return nullptr;
}

bool Task::isCompleted()
{
    return completed;
}

void Task::setCompleted(bool c)
{
    completed = c;
}

void Task::setFailed(bool f)
{
    failed = f;
}

bool Task::hasFailed()
{
    return failed;
}

bool Task::goesInTaskHistory()
{
    return recordableInHistory;
}

void Task::declareSelfAsPartOfReversion()
{
    isPartOfReversion = true;
}

void Task::preventFromGoingToTaskHistory()
{
    recordableInHistory = false;
}

void Task::prepareForRepost()
{
    completed = false;
    failed = false;
}

std::vector<std::shared_ptr<Task>> Task::getOppositeTasks()
{
    // will be nullptr
    std::vector<std::shared_ptr<Task>> emptyReversionTasks;
    return emptyReversionTasks;
}

// ============================

SilentTask::SilentTask()
{
    recordableInHistory = false;
}

// ============================

SampleCreateTask::SampleCreateTask(std::string path, int pos)
    : duplicationType(DUPLICATION_TYPE_NO_DUPLICATION), filePath(path), position(pos), isCopy(false)
{
    duplicatedSampleId = -1;
    highPassFreq = 0.0f;
    lowPassFreq = 0.0f;
    newIndex = -1;
    splitFrequency = 0.0f;
    originalLength = 0;
    reuseNewId = false;
}

SampleCreateTask::SampleCreateTask(int pos, int sampleCopyIndex, DuplicationType d)
    : duplicationType(d), position(pos), isCopy(true), duplicatedSampleId(sampleCopyIndex)
{
    highPassFreq = 0.0f;
    lowPassFreq = 0.0f;
    newIndex = -1;
    splitFrequency = 0.0f;
    originalLength = 0;
    reuseNewId = false;
}

SampleCreateTask::SampleCreateTask(float frequency, int sampleCopyIndex)
    : duplicationType(DUPLICATION_TYPE_SPLIT_AT_FREQUENCY), isCopy(true), duplicatedSampleId(sampleCopyIndex),
      splitFrequency(frequency)
{

    highPassFreq = 0.0f;
    lowPassFreq = 0.0f;
    newIndex = -1;
    originalLength = 0;
    reuseNewId = false;
}

float SampleCreateTask::getSplitFrequency()
{
    return splitFrequency;
}

DuplicationType SampleCreateTask::getDuplicationType()
{
    return duplicationType;
}

bool SampleCreateTask::isDuplication()
{
    return duplicationType != DUPLICATION_TYPE_NO_DUPLICATION;
};

int SampleCreateTask::getDuplicateTargetId()
{
    if (isCopy)
    {
        return duplicatedSampleId;
    }
    else
    {
        return 0;
    }
}

std::string SampleCreateTask::getFilePath()
{
    return filePath;
}

int64_t SampleCreateTask::getPosition()
{
    return position;
}

void SampleCreateTask::setAllocatedIndex(int i)
{
    newIndex = i;
}

int SampleCreateTask::getAllocatedIndex()
{
    return newIndex;
}

void SampleCreateTask::setSampleInitialLength(int len)
{
    originalLength = len;
}

std::vector<std::shared_ptr<Task>> SampleCreateTask::getOppositeTasks()
{
    std::vector<std::shared_ptr<Task>> reversionTasks;

    // delete the new track
    std::shared_ptr<SampleDeletionTask> deletionTask = std::make_shared<SampleDeletionTask>(newIndex);
    reversionTasks.push_back(deletionTask);

    // depending on duplication we post a second task or not.
    // Split tasks require resetting original sample.
    switch (duplicationType)
    {
    case DUPLICATION_TYPE_NO_DUPLICATION:
    case DUPLICATION_TYPE_COPY_AT_POSITION:
        break;

    case DUPLICATION_TYPE_SPLIT_AT_FREQUENCY: {
        std::shared_ptr<SampleFreqCropTask> filterRestoreTask =
            std::make_shared<SampleFreqCropTask>(false, duplicatedSampleId, splitFrequency, highPassFreq);
        reversionTasks.push_back(filterRestoreTask);
        break;
    }

    case DUPLICATION_TYPE_SPLIT_AT_POSITION:
        std::shared_ptr<SampleTimeCropTask> lengthRestoreTask =
            std::make_shared<SampleTimeCropTask>(false, duplicatedSampleId, originalLength - position);
        reversionTasks.push_back(lengthRestoreTask);
        break;
    }

    return reversionTasks;
}

void SampleCreateTask::prepareForRepost()
{
    setCompleted(false);
    setFailed(false);
    reuseNewId = true;
}

void SampleCreateTask::setFilterInitialFrequencies(float highPass, float lowPass)
{
    lowPassFreq = lowPass;
    highPassFreq = highPass;
}

std::string SampleCreateTask::marshal()
{
    json taskj = {{"object", "task"},
                  {"task", "sample_create_task"},
                  {"duplication_type", duplicationType},
                  {"filePath", filePath},
                  {"position", position},
                  {"is_copy", isCopy},
                  {"duplicated_sample_id", duplicatedSampleId},
                  {"new_index", newIndex},
                  {"split_frequency", splitFrequency},
                  {"low_pass_freq", lowPassFreq},
                  {"high_pass_freq", highPassFreq},
                  {"original_length", originalLength},
                  {"is_completed", isCompleted()},
                  {"failed", hasFailed()},
                  {"recordable_in_history", recordableInHistory},
                  {"is_part_of_reversion", isPartOfReversion}};
    return taskj.dump();
}

///////////////////////////////////////////////

NotificationTask::NotificationTask(std::string s) : message(s)
{
}

NotificationTask::NotificationTask(juce::String s) : message(s.toStdString())
{
}

std::string NotificationTask::getMessage()
{
    return message;
}

std::string NotificationTask::marshal()
{
    json taskj = {{"object", "task"},
                  {"task", "notification"},
                  {"message", message},
                  {"is_completed", isCompleted()},
                  {"failed", hasFailed()},
                  {"recordable_in_history", recordableInHistory},
                  {"is_part_of_reversion", isPartOfReversion}};
    return taskj.dump();
}

/////////////////////////////////////////////////

SampleDisplayTask::SampleDisplayTask(std::shared_ptr<SamplePlayer> sp, std::shared_ptr<SampleCreateTask> crTask)
{
    sample = sp;
    creationTask = crTask;
}

std::string SampleDisplayTask::marshal()
{
    json taskj = {{"object", "task"},
                  {"task", "sample_display_task"},
                  {"is_completed", isCompleted()},
                  {"failed", hasFailed()},
                  {"recordable_in_history", recordableInHistory},
                  {"is_part_of_reversion", isPartOfReversion}};
    return taskj.dump();
}

/////////////////////////////////////////////////

ImportFileCountTask::ImportFileCountTask(std::string p)
{
    path = p;
}

/////////////////////////////////////////////////

SampleDeletionDisplayTask::SampleDeletionDisplayTask(int i)
{
    id = i;
}

/////////////////////////////////////////////////

SampleDeletionTask::SampleDeletionTask(int i)
{
    id = i;
}

std::vector<std::shared_ptr<Task>> SampleDeletionTask::getOppositeTasks()
{
    std::vector<std::shared_ptr<Task>> response;
    std::shared_ptr<SampleRestoreTask> restoreTask = std::make_shared<SampleRestoreTask>(id, deletedSample);
    response.push_back(restoreTask);
    return response;
}

std::string SampleDeletionTask::marshal()
{
    json taskj = {{"object", "task"},
                  {"task", "sample_deletion"},
                  {"id_to_delete", id},
                  {"is_completed", isCompleted()},
                  {"failed", hasFailed()},
                  {"recordable_in_history", recordableInHistory},
                  {"is_part_of_reversion", isPartOfReversion}};
    return taskj.dump();
}

/////////////////////////////////////////////////

SampleRestoreTask::SampleRestoreTask(int i, std::shared_ptr<SamplePlayer> sample)
{
    id = i;
    sampleToRestore = sample;
}

std::string SampleRestoreTask::marshal()
{
    json taskj = {{"object", "task"},
                  {"task", "sample_restore"},
                  {"id_to_restore", id},
                  {"is_completed", isCompleted()},
                  {"failed", hasFailed()},
                  {"recordable_in_history", recordableInHistory},
                  {"is_part_of_reversion", isPartOfReversion}};
    return taskj.dump();
}

/////////////////////////////////////////////////

SampleRestoreDisplayTask::SampleRestoreDisplayTask(int i, std::shared_ptr<SamplePlayer> sample)
{
    id = i;
    restoredSample = sample;
}

/////////////////////////////////////////////////

SampleTimeCropTask::SampleTimeCropTask(bool cropBeginning, int sampleId, int frameDist)
    : id(sampleId), dragDistance(frameDist), movingBeginning(cropBeginning)
{
}

std::string SampleTimeCropTask::marshal()
{
    json taskj = {{"object", "task"},
                  {"task", "sample_time_crop"},
                  {"id", id},
                  {"drag_distance", dragDistance},
                  {"moving_start", movingBeginning},
                  {"is_completed", isCompleted()},
                  {"failed", hasFailed()},
                  {"recordable_in_history", recordableInHistory},
                  {"is_part_of_reversion", isPartOfReversion}};
    return taskj.dump();
}

std::vector<std::shared_ptr<Task>> SampleTimeCropTask::getOppositeTasks()
{
    std::vector<std::shared_ptr<Task>> result;
    std::shared_ptr<SampleTimeCropTask> task = std::make_shared<SampleTimeCropTask>(movingBeginning, id, -dragDistance);
    result.push_back(task);
    return result;
}

/////////////////////////////////////////////////

SampleFreqCropTask::SampleFreqCropTask(bool isLP, int sampleId, float initialFreq, float finalFreq)
    : id(sampleId), initialFrequency(initialFreq), finalFrequency(finalFreq), isLowPass(isLP)
{
}

std::string SampleFreqCropTask::marshal()
{
    json taskj = {{"object", "task"},
                  {"task", "sample_freq_crop"},
                  {"id", id},
                  {"is_low_pass", isLowPass},
                  {"initial_freq", initialFrequency},
                  {"final_freq", finalFrequency},
                  {"is_completed", isCompleted()},
                  {"failed", hasFailed()},
                  {"recordable_in_history", recordableInHistory},
                  {"is_part_of_reversion", isPartOfReversion}};
    return taskj.dump();
}

std::vector<std::shared_ptr<Task>> SampleFreqCropTask::getOppositeTasks()
{
    std::vector<std::shared_ptr<Task>> result;
    std::shared_ptr<SampleFreqCropTask> task =
        std::make_shared<SampleFreqCropTask>(isLowPass, id, finalFrequency, initialFrequency);
    result.push_back(task);
    return result;
}

/////////////////////////////////////////////////

SampleMovingTask::SampleMovingTask(int sampleId, int frameDist) : id(sampleId), dragDistance(frameDist)
{
}

std::string SampleMovingTask::marshal()
{
    json taskj = {{"object", "task"},
                  {"task", "sample_moving"},
                  {"id", id},
                  {"drag_distance", dragDistance},
                  {"is_completed", isCompleted()},
                  {"failed", hasFailed()},
                  {"recordable_in_history", recordableInHistory},
                  {"is_part_of_reversion", isPartOfReversion}};
    return taskj.dump();
}

std::vector<std::shared_ptr<Task>> SampleMovingTask::getOppositeTasks()
{
    std::vector<std::shared_ptr<Task>> result;
    std::shared_ptr<SampleMovingTask> task = std::make_shared<SampleMovingTask>(id, -dragDistance);
    result.push_back(task);
    return result;
}

/////////////////////////////////////////////////

SampleUpdateTask::SampleUpdateTask(int sampleId, std::shared_ptr<SamplePlayer> sp)
{
    sample = sp;
    id = sampleId;
}

std::string SampleUpdateTask::marshal()
{
    json taskj = {{"object", "task"},
                  {"task", "sample_update"},
                  {"id", id},
                  {"is_completed", isCompleted()},
                  {"failed", hasFailed()},
                  {"recordable_in_history", recordableInHistory},
                  {"is_part_of_reversion", isPartOfReversion}};
    return taskj.dump();
}

//////////////////////////////////////////////////////

SampleGroupRecolor::SampleGroupRecolor(int cid) : colorId(cid), colorFromId(true)
{
}

SampleGroupRecolor::SampleGroupRecolor(std::set<int> ids, std::map<int, juce::Colour> newColors)
    : colorId(-1), changedSampleIds(ids), colorsPerId(newColors), colorFromId(false)
{
}

std::string SampleGroupRecolor::marshal()
{
    std::vector<int> ids(changedSampleIds.begin(), changedSampleIds.end());
    json taskj = {{"object", "task"},
                  {"task", "sample_group_recolor"},
                  {"color_id", colorId},
                  {"changed_samples", ids},
                  {"newColor", newColor.toString().toStdString()},
                  {"color_from_id", colorFromId},
                  {"is_completed", isCompleted()},
                  {"failed", hasFailed()},
                  {"recordable_in_history", recordableInHistory},
                  {"is_part_of_reversion", isPartOfReversion}};
    return taskj.dump();
}

std::vector<std::shared_ptr<Task>> SampleGroupRecolor::getOppositeTasks()
{
    std::vector<std::shared_ptr<Task>> tasks;
    std::shared_ptr<SampleGroupRecolor> task = std::make_shared<SampleGroupRecolor>(changedSampleIds, colorsPerId);
    task->colorId = -1;
    tasks.push_back(task);
    return tasks;
}

//////////////////////////////////////////////////////

SelectionChangingTask::SelectionChangingTask(std::set<size_t> &newSelection)
{
    newSelectedTracks = newSelection;
}

std::string SelectionChangingTask::marshal()
{
    std::vector<int> ids(newSelectedTracks.begin(), newSelectedTracks.end());
    json taskj = {{"object", "task"},
                  {"task", "selection_changing"},
                  {"changed_samples", ids},
                  {"is_completed", isCompleted()},
                  {"failed", hasFailed()},
                  {"recordable_in_history", recordableInHistory},
                  {"is_part_of_reversion", isPartOfReversion}};
    return taskj.dump();
}

//////////////////////////////////////////////////////////////

NumericInputUpdateTask::NumericInputUpdateTask(int inputId, float val)
{
    recordableInHistory = false;
    newValue = val;
    numericalInputId = inputId;
    isBroadcastRequest = false;
}

NumericInputUpdateTask::NumericInputUpdateTask(int inputId, float val, float oldval)
{
    setCompleted(true);
    recordableInHistory = true;
    newValue = val;
    numericalInputId = inputId;
    oldValue = oldval;
    isBroadcastRequest = false;
}

NumericInputUpdateTask::NumericInputUpdateTask(int id)
{
    recordableInHistory = false;
    newValue = 0;
    oldValue = 0;
    numericalInputId = id;
    isBroadcastRequest = true;
}

std::vector<std::shared_ptr<Task>> NumericInputUpdateTask::getOppositeTasks()
{
    std::vector<std::shared_ptr<Task>> tasks;
    // do nothing if it's not meant to be reverted.
    // tbh the reversion call shouldn't even happen anyway but who knows.
    if (!recordableInHistory)
    {
        return tasks;
    }

    std::shared_ptr<NumericInputUpdateTask> task =
        std::make_shared<NumericInputUpdateTask>(numericalInputId, oldValue, newValue);
    // this constructor default to completed task! we gotta set it back to uncompleted
    task->setCompleted(false);

    tasks.push_back(task);
    return tasks;
}

std::string NumericInputUpdateTask::marshal()
{
    json taskj = {{"object", "task"},
                  {"task", "numeric_input_update"},
                  {"new_value", newValue},
                  {"old_value", oldValue},
                  {"numeric_input_id", numericalInputId},
                  {"is_broadcast_request", isBroadcastRequest},
                  {"is_completed", isCompleted()},
                  {"failed", hasFailed()},
                  {"recordable_in_history", recordableInHistory},
                  {"is_part_of_reversion", isPartOfReversion}};
    return taskj.dump();
}

////////////////////////////////

PlayStateUpdateTask::PlayStateUpdateTask()
{
    requestingStateBroadcast = true;
    shouldPlay = false;
    shouldResetPosition = false;
    isCurrentlyPlaying = false;
}

PlayStateUpdateTask::PlayStateUpdateTask(bool play, bool resetPosition)
{
    requestingStateBroadcast = false;
    shouldPlay = play;
    shouldResetPosition = resetPosition;
    isCurrentlyPlaying = false;
}

std::string PlayStateUpdateTask::marshal()
{
    json taskj = {{"object", "task"},
                  {"task", "play_state_update"},
                  {"requesting_state_broadcast", requestingStateBroadcast},
                  {"should_play", shouldPlay},
                  {"should_reset_position", shouldResetPosition},
                  {"is_currently_playing", isCurrentlyPlaying},
                  {"is_completed", isCompleted()},
                  {"failed", hasFailed()},
                  {"recordable_in_history", recordableInHistory},
                  {"is_part_of_reversion", isPartOfReversion}};
    return taskj.dump();
}

/////////////////////////////////

LoopToggleTask::LoopToggleTask()
{
    requestingStateBroadcast = true;
    shouldLoop = false;
    isCurrentlyLooping = false;
}

LoopToggleTask::LoopToggleTask(bool on)
{
    requestingStateBroadcast = false;
    shouldLoop = on;
    isCurrentlyLooping = false;
}

std::string LoopToggleTask::marshal()
{
    json taskj = {{"object", "task"},
                  {"task", "loop_mode_toggle"},
                  {"requesting_state_broadcast", requestingStateBroadcast},
                  {"should_loop", shouldLoop},
                  {"is_currently_lopping", isCurrentlyLooping},
                  {"is_completed", isCompleted()},
                  {"failed", hasFailed()},
                  {"recordable_in_history", recordableInHistory},
                  {"is_part_of_reversion", isPartOfReversion}};
    return taskj.dump();
}

/////////////////////////////////////////

LoopMovingTask::LoopMovingTask()
{
    isBroadcastRequest = true;
    recordableInHistory = false;
    previousLoopBeginFrame = 0;
    previousLoopEndFrame = 0;
    currentLoopBeginFrame = 0;
    currentLoopEndFrame = 0;
}

LoopMovingTask::LoopMovingTask(int64_t newBegin, int64_t newEnd)
{
    isBroadcastRequest = false;
    recordableInHistory = false;
    previousLoopBeginFrame = 0;
    previousLoopEndFrame = 0;
    currentLoopBeginFrame = newBegin;
    currentLoopEndFrame = newEnd;
}

LoopMovingTask::LoopMovingTask(int64_t oldBegin, int64_t oldEnd, int64_t newBegin, int64_t newEnd)
{
    isBroadcastRequest = false;
    recordableInHistory = true;
    previousLoopBeginFrame = oldBegin;
    previousLoopEndFrame = oldEnd;
    currentLoopBeginFrame = newBegin;
    currentLoopEndFrame = newEnd;
}

std::string LoopMovingTask::marshal()
{
    json taskj = {{"object", "task"},
                  {"task", "loop_moving"},
                  {"is_broadcast_request", isBroadcastRequest},
                  {"previous_loop_begin_frame", previousLoopBeginFrame},
                  {"previous_loop_end_frame", previousLoopEndFrame},
                  {"current_loop_begin_frame", currentLoopBeginFrame},
                  {"current_loop_end_frame", currentLoopEndFrame},
                  {"is_completed", isCompleted()},
                  {"failed", hasFailed()},
                  {"recordable_in_history", recordableInHistory},
                  {"is_part_of_reversion", isPartOfReversion}};
    return taskj.dump();
}