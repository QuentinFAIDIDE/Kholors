#ifndef DEF_ACTIVITY_MANAGER_HPP
#define DEF_ACTIVITY_MANAGER_HPP

#include "../Audio/MixingBus.h"
#include "../UserInterface/ArrangementArea.h"
#include "./Action.h"

class ActivityManager {
public:
    ActivityManager();
    ~Activitymanager();
    AppState& getAppState();

private:
    std::vector<Action> history;
    AppState appState;
};

#endif // DEF_ACTIVITY_MANAGER_HPP
