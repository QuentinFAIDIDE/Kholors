#include "Task.h"
#include "TaxonomyManager.h"
#include <memory>

Task::Task()
{
    completed = false;
    failed = false;
    recordableInHistory = false;
    reversed = false;
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

std::vector<std::shared_ptr<Task>> Task::getReversed()
{
    std::vector<std::shared_ptr<Task>> emptyReversionTasks;
    return emptyReversionTasks;
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

    recordableInHistory = true;
}

SampleCreateTask::SampleCreateTask(int pos, int sampleCopyIndex, DuplicationType d)
    : duplicationType(d), position(pos), isCopy(true), duplicatedSampleId(sampleCopyIndex)
{
    highPassFreq = 0.0f;
    lowPassFreq = 0.0f;
    newIndex = -1;
    splitFrequency = 0.0f;
    originalLength = 0;

    recordableInHistory = true;
}

SampleCreateTask::SampleCreateTask(float frequency, int sampleCopyIndex)
    : duplicationType(DUPLICATION_TYPE_SPLIT_AT_FREQUENCY), isCopy(true), duplicatedSampleId(sampleCopyIndex),
      splitFrequency(frequency)
{

    highPassFreq = 0.0f;
    lowPassFreq = 0.0f;
    newIndex = -1;
    originalLength = 0;

    recordableInHistory = true;
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

std::vector<std::shared_ptr<Task>> SampleCreateTask::getReversed()
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
            std::make_shared<SampleTimeCropTask>(position, duplicatedSampleId, originalLength - position);
        reversionTasks.push_back(lengthRestoreTask);
        break;
    }

    return reversionTasks;
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
                  {"reversed", reversed}};
    return taskj.dump();
}

///////////////////////////////////////////////

NotificationTask::NotificationTask(std::string s) : message(s)
{
    recordableInHistory = false;
}

NotificationTask::NotificationTask(juce::String s) : message(s.toStdString())
{
    recordableInHistory = false;
}

std::string NotificationTask::getMessage()
{
    return message;
}

std::string NotificationTask::marshal()
{
    json taskj = {{"object", "task"},      {"task", "notification"},
                  {"message", message},    {"is_completed", isCompleted()},
                  {"failed", hasFailed()}, {"recordable_in_history", recordableInHistory},
                  {"reversed", reversed}};
    return taskj.dump();
}

/////////////////////////////////////////////////

SampleDisplayTask::SampleDisplayTask(std::shared_ptr<SamplePlayer> sp, std::shared_ptr<SampleCreateTask> crTask)
{
    sample = sp;
    creationTask = crTask;

    recordableInHistory = false;
}

std::string SampleDisplayTask::marshal()
{
    json taskj = {{"object", "task"},
                  {"task", "sample_display_task"},
                  {"is_completed", isCompleted()},
                  {"failed", hasFailed()},
                  {"recordable_in_history", recordableInHistory},
                  {"reversed", reversed}};
    return taskj.dump();
}

/////////////////////////////////////////////////

ImportFileCountTask::ImportFileCountTask(std::string p)
{
    path = p;

    recordableInHistory = false;
}

/////////////////////////////////////////////////

SampleDeletionDisplayTask::SampleDeletionDisplayTask(int i)
{
    id = i;

    recordableInHistory = false;
}

/////////////////////////////////////////////////

SampleDeletionTask::SampleDeletionTask(int i)
{
    id = i;
    recordableInHistory = true;
}

std::vector<std::shared_ptr<Task>> SampleDeletionTask::getReversed()
{
    std::vector<std::shared_ptr<Task>> response;
    std::shared_ptr<SampleRestoreTask> restoreTask = std::make_shared<SampleRestoreTask>(id, deletedSample);
    response.push_back(restoreTask);
    return response;
}

std::string SampleDeletionTask::marshal()
{
    json taskj = {{"object", "task"},      {"task", "sample_deletion"},
                  {"id_to_delete", id},    {"is_completed", isCompleted()},
                  {"failed", hasFailed()}, {"recordable_in_history", recordableInHistory},
                  {"reversed", reversed}};
    return taskj.dump();
}

/////////////////////////////////////////////////

SampleRestoreTask::SampleRestoreTask(int i, std::shared_ptr<SamplePlayer> sample)
{
    id = i;
    sampleToRestore = sample;
    recordableInHistory = true;
}

std::string SampleRestoreTask::marshal()
{
    json taskj = {{"object", "task"},      {"task", "sample_restore"},
                  {"id_to_restore", id},   {"is_completed", isCompleted()},
                  {"failed", hasFailed()}, {"recordable_in_history", recordableInHistory},
                  {"reversed", reversed}};
    return taskj.dump();
}

/////////////////////////////////////////////////

SampleRestoreDisplayTask::SampleRestoreDisplayTask(int i, std::shared_ptr<SamplePlayer> sample)
{
    id = i;
    restoredSample = sample;
    recordableInHistory = false;
}

/////////////////////////////////////////////////

SampleTimeCropTask::SampleTimeCropTask(bool cropBeginning, int sampleId, int frameDist)
    : id(sampleId), dragDistance(frameDist), movingBeginning(cropBeginning)
{
    recordableInHistory = true;
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
                  {"reversed", reversed}};
    return taskj.dump();
}

std::vector<std::shared_ptr<Task>> SampleTimeCropTask::getReversed()
{
    std::vector<std::shared_ptr<Task>> result;
    std::shared_ptr<SampleTimeCropTask> task = std::make_shared<SampleTimeCropTask>(movingBeginning, id, -dragDisance);
    result.push_back(task);
    return result;
}

/////////////////////////////////////////////////

SampleFreqCropTask::SampleFreqCropTask(bool isLP, int sampleId, float initialFreq, float finalFreq)
    : id(sampleId), initialFrequency(initialFreq), finalFrequency(finalFreq), isLowPass(isLP)
{
    recordableInHistory = true;
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
                  {"reversed", reversed}};
    return taskj.dump();
}

std::vector<std::shared_ptr<Task>> SampleFreqCropTask::getReversed()
{
    std::vector<std::shared_ptr<Task>> result;
    std::shared_ptr<SampleFreqCropTask> task = std::make_shared<SampleFreqCropTask>(isLowPass, id, finalFreq, initialFreq);
    result.push_back(task);
    return result;
}

/////////////////////////////////////////////////

SampleMovingTask::SampleMovingTask(int sampleId, int frameDist) : id(sampleId), dragDistance(frameDist)
{
    recordableInHistory = true;
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
                  {"reversed", reversed}};
    return taskj.dump();
}

std::vector<std::shared_ptr<Task>> SampleMovingTask::getReversed()
{
    std::vector<std::shared_ptr<Task>> result;
    std::shared_ptr<SampleMovingTask> task = std::make_shared<SampleMovingTask>(id, -dragDistance);
    result.push_back(task);
    return result;
}

/////////////////////////////////
SampleUpdateTask::SampleUpdateTask(int sampleId, std::shared_ptr<SamplePlayer> sp)
{
    sample = sp;
    id = sampleId;
    recordableInHistory = false;
}

std::string SampleUpdateTask::marshal()
{
    json taskj = {{"object", "task"},
                  {"task", "sample_view_update"},
                  {"id", id},
                  {"is_completed", isCompleted()},
                  {"failed", hasFailed()},
                  {"recordable_in_history", recordableInHistory},
                  {"reversed", reversed}};
    return taskj.dump();
}

