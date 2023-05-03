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

    // TODO: only allow message thread

    const juce::SpinLock::ScopedTryLockType lock(broadcastLock);
    if (taskBroadcastStopped)
    {
        return;
    }

    {
        const juce::SpinLock::ScopedLockType queueLock(taskQueueLock);
        taskQueue.push(task);
    }

    // only one broadcast call will iterate over items at any time
    if (lock.isLocked())
    {
        std::shared_ptr<Task> taskToBroadcast(nullptr);
        
        {
            const juce::SpinLock::ScopedLockType queueLock(taskQueueLock);
            taskToBroadcast = taskQueue.front();
            taskQueue.pop();
        }

        while (taskToBroadcast != nullptr)
        {
            for (size_t i=0; i < taskListeners.size(); i++)
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
}

void ActivityManager::registerTaskListener(TaskListener* newListener)
{
    const juce::SpinLock::ScopedLockType lock(broadcastLock);
    taskListeners.push_back(newListener); 
}