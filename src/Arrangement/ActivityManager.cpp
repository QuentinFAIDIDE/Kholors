#include "ActivityManager.h"

ActivityManager::ActivityManager()
{
    taskBroadcastStopped = false;
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

void ActivityManager::broadcastTask(std::shared_ptr<Task> task)
{
    const juce::SpinLock::ScopedTryLockType lock(broadcastLock);
    if (taskBroadcastStopped)
    {
        return;
    }

    {
        const juce::SpinLock::ScopedLockType queueLock(taskQueueLock);
        taskQueue.push(task);
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

    for (size_t i = 0; i < taskListeners.size(); i++)
    {
        bool shouldStop = taskListeners[i]->taskHandler(priorityTask);
        if (shouldStop)
        {
            break;
        }
    }
}