#include "ActivityManager.h"
#include <memory>

ActivityManager::ActivityManager()
{
    taskBroadcastStopped = false;
    historyNextIndex = 0;
}

ActivityManager::~ActivityManager()
{
    // TODO
}

AppState &ActivityManager::getAppState()
{
    return appState;
}

void ActivityManager::stopTaskBroadcast()
{
    const juce::SpinLock::ScopedLockType lock(broadcastLock);
    taskBroadcastStopped = true;
}

void ActivityManager::broadcastTask(std::shared_ptr<Task> submittedTask)
{
    const juce::SpinLock::ScopedTryLockType lock(broadcastLock);
    if (taskBroadcastStopped)
    {
        return;
    }

    {
        const juce::SpinLock::ScopedLockType queueLock(taskQueueLock);
        taskQueue.push(submittedTask);
    }

    if (!lock.isLocked())
    {
        return;
    }

    std::shared_ptr<Task> taskToBroadcast(nullptr);

    // all broadcasted tasks arrive on the message thread
    const juce::MessageManagerLock mmLock;

    {
        const juce::SpinLock::ScopedLockType queueLock(taskQueueLock);
        taskToBroadcast = taskQueue.front();
        taskQueue.pop();
    }

    while (taskToBroadcast != nullptr)
    {
        for (size_t i = 0; i < taskListeners.size(); i++)
        {
            bool shouldStop = taskListeners[i]->taskHandler(taskToBroadcast);
            if (shouldStop)
            {
                break;
            }
        }

        if (taskToBroadcast->goesInTaskHistory() && taskToBroadcast->isCompleted() && !taskToBroadcast->hasFailed())
        {
            recordTaskInHistory(taskToBroadcast);
        }
        else
        {
            std::cout << "Unrecorded task: " << taskToBroadcast->marshal() << std::endl;
        }

        taskToBroadcast = std::shared_ptr<Task>(nullptr);
        {
            const juce::SpinLock::ScopedLockType queueLock(taskQueueLock);
            if (!taskQueue.empty())
            {
                taskToBroadcast = taskQueue.front();
                taskQueue.pop();
            }
        }
    }
}

void ActivityManager::registerTaskListener(TaskListener *newListener)
{
    const juce::SpinLock::ScopedLockType lock(broadcastLock);
    taskListeners.push_back(newListener);
}

void ActivityManager::broadcastNestedTaskNow(std::shared_ptr<Task> priorityTask)
{
    // yet another warning: don't call this function from anything else
    // than an already running taskHandler !

    // NOTE: we don't record nested tasks in history

    for (size_t i = 0; i < taskListeners.size(); i++)
    {
        bool shouldStop = taskListeners[i]->taskHandler(priorityTask);
        if (shouldStop)
        {
            break;
        }
    }
}

void ActivityManager::recordTaskInHistory(std::shared_ptr<Task> taskToRecord)
{
    // This should only be called by the broadcastTask function
    // when it has lock message thread and broadcasting lock.

    // we always empty the canceled task stack
    // after a new task is performed to
    // we ensure "ctrl + shift + z" type calls
    // won't restore past activity
    canceledTasks.clear();

    history[historyNextIndex] = taskToRecord;
    historyNextIndex = (historyNextIndex + 1) % ACTIVITY_HISTORY_RING_BUFFER_SIZE;

    std::cout << "Recorded task: " << taskToRecord->marshal() << std::endl;
}

void ActivityManager::undoLastActivity()
{
    juce::SpinLock::ScopedLockType bLock(broadcastLock);

    int lastActivityIndex = (historyNextIndex - 1) % ACTIVITY_HISTORY_RING_BUFFER_SIZE;
    auto revertedTasks = history[lastActivityIndex]->getReversed();

    if (history[lastActivityIndex] == nullptr || revertedTasks.size() == 0)
    {
        return;
    }

    for (size_t i = 0; i < revertedTasks.size(); i++)
    {
        for (size_t j = 0; j < taskListeners.size(); j++)
        {
            bool shouldStop = taskListeners[j]->taskHandler(revertedTasks[i]);
            if (shouldStop)
            {
                break;
            }
        }
    }

    canceledTasks.push_back(history[lastActivityIndex]);
    history[lastActivityIndex] = nullptr;
    historyNextIndex = lastActivityIndex;
}