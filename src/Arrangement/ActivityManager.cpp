#include "ActivityManager.h"

ActivityManager::ActivityManager()
{
    // TODO
}

ActivityManager::~ActivityManager()
{
    // TODO
}

AppState &ActivityManager::getAppState()
{
    return appState;
}

void ActivityManager::broadcastTask(std::shared_ptr<Task> task)
{

    // TODO: only allow message thread

    const juce::SpinLock::ScopedTryLockType lock(broadcastLock);

    {
        const juce::SpinLock::ScopedLockType lock(taskQueueLock);
        taskQueue.push(task);
    }

    // only one broadcast call will iterate over items at any time
    if (lock.isLocked())
    {
        std::shared_ptr<Task> taskToBroadcast = nullptr;
        
        {
            const juce::SpinLock::ScopedLockType lock(taskQueueLock);
            taskToBroadcast = taskQueue.pop();
        }

        while (taskToBroadcast != nullptr)
        {
            for (size_t i=0; i < taskListeners.size(); i++)
            {
                bool shouldStop = taskListeners[i].taskHandler(taskToBroadcast);
                if (shouldStop)
                {
                    break;
                }
            }

            taskToBroadcast = nullptr;
            {
                const juce::SpinLock::ScopedLockType lock(taskQueueLock);
                if (!taskQueue.empty())
                {
                    taskToBroadcast = taskQueue.pop();
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