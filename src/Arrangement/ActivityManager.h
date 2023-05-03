#ifndef DEF_ACTIVITY_MANAGER_HPP
#define DEF_ACTIVITY_MANAGER_HPP

#include <vector>

#include "./AppState.h"
#include "./Task.h"

class TaskListener {
public:
  /**
  * Returns true if the task don't need further broadcast.
  */
  virtual bool taskHandler(std::shared_ptr<Task> task) = 0;
}

class ActivityManager
{
  public:
    ActivityManager();
    ~ActivityManager();
    AppState &getAppState();
    void registerTaskListener(TaskListener*);
    void stopTaskBroadcast();
    
    /**
    * Broadcast the task to all listeners.
    */
    void broadcastTask(std::shared_ptr<Task>);

  private:
    std::vector<Task> history;
    AppState appState;
    std::vector<TaskListener*> taskListeners;
    std::queue<std::shared_ptr<Task>> taskQueue;
    juce::SpinLock taskQueueLock;
    juce::SpinLock broadcastLock;
    bool taskBroadcastStopped;
};

#endif // DEF_ACTIVITY_MANAGER_HPP
