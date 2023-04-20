#ifndef DEF_ACTIVITY_MANAGER_HPP
#define DEF_ACTIVITY_MANAGER_HPP

#include "../Audio/MixingBus.h"
#include "../UserInterface/ArrangementArea.h"

class ActivityManager {
public:
    ActivityManager();
    ~Activitymanager();

private:
    std::vector<Action> history;
    AppState appState;
};

#endif // DEF_ACTIVITY_MANAGER_HPP
