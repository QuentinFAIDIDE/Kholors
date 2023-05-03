#ifndef DEF_ACTIVITY_MANAGER_HPP
#define DEF_ACTIVITY_MANAGER_HPP

#include <vector>

#include "./AppState.h"
#include "./Task.h"

class ActivityManager
{
  public:
    ActivityManager();
    ~ActivityManager();
    AppState &getAppState();
    
    /**
    * Copy the task at the pointer and broadcast it
    * to listeners.
    */
    void broadcastTask(Task*);

  private:
    std::vector<Task> history;
    AppState appState;
};

#endif // DEF_ACTIVITY_MANAGER_HPP
