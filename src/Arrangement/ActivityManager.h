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

  private:
    std::vector<Task> history;
    AppState appState;
};

#endif // DEF_ACTIVITY_MANAGER_HPP
