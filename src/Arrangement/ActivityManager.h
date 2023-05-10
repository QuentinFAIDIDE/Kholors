#ifndef DEF_ACTIVITY_MANAGER_HPP
#define DEF_ACTIVITY_MANAGER_HPP

#include <memory>
#include <stack>
#include <vector>

#include "./AppState.h"
#include "./Task.h"

class TaskListener
{
  public:
    /**
     * Returns true if the task don't need further broadcast.
     */
    virtual bool taskHandler(std::shared_ptr<Task> task) = 0;
};

class ActivityManager
{
  public:
    /**
     Creates a new activity manager
     */
    ActivityManager();

    /**
     Deletes the activity manager
     */
    ~ActivityManager();

    /**
     * Return a reference to the AppState object.
     * @return a reference to the AppState object.
     */
    AppState &getAppState();

    /**
     Add the TaskListener class to a list of objects which gets
     their callback called with the tasks when they are broadcasted.
     */
    void registerTaskListener(TaskListener *);

    /**
       Take the task broadcasting lock and set its status
       to stopped so no further task is processed.
     */
    void stopTaskBroadcast();

    /**
     Broadcast the task to all listeners.
     */
    void broadcastTask(std::shared_ptr<Task>);

    /**
     This should only be called from an already running taskHandler.
     Will call another taskHandler and jump the queue of tasks.
     */
    void broadcastNestedTaskNow(std::shared_ptr<Task>);

    /**
     Append this task to history ring buffer.
     */
    void recordTaskInHistory(std::shared_ptr<Task>);

    /**
     Undo the last activity.
     */
    void undoLastActivity();

    /**
     Redo the stored last activities that were undone.
     */
    void redoLastActivity();

  private:
    std::shared_ptr<Task> history[ACTIVITY_HISTORY_RING_BUFFER_SIZE]; // ring buffer with the last executed tasks
    int historyNextIndex;                                             // the index of the next recorded history entry
    std::stack<std::shared_ptr<Task>> canceledTasks;                  // stack of canceled tasks

    AppState appState; // an object representing the application state

    std::vector<TaskListener *> taskListeners;   // a list of object we broadcast tasks to
    std::queue<std::shared_ptr<Task>> taskQueue; // the queue of tasks to be broadcasted
    juce::SpinLock taskQueueLock;                // the lock to prevent race condition on the task queue
    juce::SpinLock broadcastLock;                // the lock to prevent concurrent broadcast of tasks
    bool taskBroadcastStopped;                   // boolean to tell if the broadcasting processing is enabled
};

#endif // DEF_ACTIVITY_MANAGER_HPP
