#include "Task.h"
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

std::string Task::Marshal()
{
    return "none";
}

Marshalable *Task::Unmarshal(std::string &)
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

SampleCreateTask::SampleCreateTask(std::string path, int position)
    : filePath(path), editingPosition(position), isCopy(false), duplicationType(DUPLICATION_TYPE_NO_DUPLICATION)
{
    recordableInHistory = true;
}

SampleCreateTask::SampleCreateTask(int position, int sampleCopyIndex, DuplicationType d)
    : editingPosition(position), isCopy(true), duplicatedSampleId(sampleCopyIndex), duplicationType(d)
{
    recordableInHistory = true;
}

SampleCreateTask::SampleCreateTask(float frequency, int sampleCopyIndex)
    : splitFrequency(frequency), isCopy(true), duplicatedSampleId(sampleCopyIndex),
      duplicationType(DUPLICATION_TYPE_SPLIT_AT_FREQUENCY)
{
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
    return editingPosition;
}

void SampleCreateTask::setAllocatedIndex(int i)
{
    newIndex = i;
}

int SampleCreateTask::getAllocatedIndex()
{
    return newIndex;
}

std::vector<std::shared_ptr<Task>> SampleCreateTask::getReversed()
{
    // TODO: return a set of tasks that are reverting this
    std::vector<std::shared_ptr<Task>> emptyReversionTasks;
    return emptyReversionTasks;
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

/////////////////////////////////////////////////

SampleDisplayTask::SampleDisplayTask(std::shared_ptr<SamplePlayer> sp, std::shared_ptr<SampleCreateTask> crTask)
{
    sample = sp;
    creationTask = crTask;

    recordableInHistory = false;
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

/////////////////////////////////////////////////

SampleTimeCropTask::SampleTimeCropTask(bool cropBeginning, int sampleId, int frameDist)
    : movedBeginning(cropBeginning), id(sampleId), dragDistance(frameDist)
{
    recordableInHistory = true;
}

/////////////////////////////////////////////////

SampleFreqCropTask::SampleFreqCropTask(bool isLP, int sampleId, float initialFreq, float finalFreq)
    : isLowPass(isLP), id(sampleId), initialFrequency(initialFreq), finalFrequency(finalFreq)
{
    recordableInHistory = true;
}

/////////////////////////////////////////////////

SampleMovingTask::SampleMovingTask(int sampleId, int frameDist) : id(sampleId), dragDistance(frameDist)
{
    recordableInHistory = true;
}