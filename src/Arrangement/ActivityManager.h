#ifndef DEF_ACTIVITY_MANAGER_HPP
#define DEF_ACTIVITY_MANAGER_HPP

#include <vector>

#include "./Action.h"
#include "./AppState.h"

class ActivityManager
{
  public:
    ActivityManager();
    ~ActivityManager();
    AppState &getAppState();

  private:
    std::vector<Action> history;
    AppState appState;
};

#endif // DEF_ACTIVITY_MANAGER_HPP
