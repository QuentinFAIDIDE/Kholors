#include "Task.h"
#include <memory>

Task::Task()
{
    completed = false;
    failed = false;
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

// ============================

SampleCreateTask::SampleCreateTask(std::string path, int position)
    : filePath(path), editingPosition(position), isCopy(false), duplicationType(DUPLICATION_TYPE_NO_DUPLICATION)
{
}

SampleCreateTask::SampleCreateTask(int position, int sampleCopyIndex, DuplicationType d)
    : editingPosition(position), isCopy(true), duplicatedSampleId(sampleCopyIndex), duplicationType(d)
{
}

SampleCreateTask::SampleCreateTask(float frequency, int sampleCopyIndex)
    : splitFrequency(frequency), isCopy(true), duplicatedSampleId(sampleCopyIndex),
      duplicationType(DUPLICATION_TYPE_SPLIT_AT_FREQUENCY)
{
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

/////////////////////////////////////////////////

SampleDisplayTask::SampleDisplayTask(SamplePlayer *sp, std::shared_ptr<SampleCreateTask> crTask)
{
    sample = sp;
    creationTask = crTask;
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

/////////////////////////////////////////////////

SampleTimeCropTask::SampleTimeCropTask(bool cropBeginning, int sampleId, int frameDist)
    : movedBeginning(cropBeginning), id(sampleId), dragDistance(frameDist)
{
}

/////////////////////////////////////////////////

SampleFreqCropTask::SampleFreqCropTask(bool isLP, int sampleId, float initialFreq, float finalFreq)
    : isLowPass(isLP), id(sampleId), initialFrequency(initialFreq), finalFrequency(finalFreq)
{
}

/////////////////////////////////////////////////

SampleMovingTask::SampleMovingTask(int sampleId, int frameDist) : id(sampleId), dragDistance(frameDist)
{
}