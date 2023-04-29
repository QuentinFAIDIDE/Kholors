#include "Task.h"

Task::Task()
{
    completed = false;
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

// ============================

SampleImportTask::SampleImportTask(std::string path, int position)
    : filePath(path), editingPosition(position), isCopy(false), failed(false)
{
}

SampleImportTask::SampleImportTask(int position, int sampleCopyIndex)
    : editingPosition(position), isCopy(true), duplicatedSampleId(sampleCopyIndex), failed(false)
{
}

bool SampleImportTask::isDuplication()
{
    return isCopy;
};

int SampleImportTask::getDuplicateTargetId()
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

std::string SampleImportTask::getFilePath()
{
    return filePath;
}

int64_t SampleImportTask::getPosition()
{
    return editingPosition;
}

void SampleImportTask::setFailed(bool f)
{
    failed = f;
}

bool SampleImportTask::hasFailed()
{
    return failed;
}

void SampleImportTask::setAllocatedIndex(int i)
{
    newIndex = i;
}

int SampleImportTask::getAllocatedIndex()
{
    return newIndex;
}