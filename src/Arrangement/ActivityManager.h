#ifndef DEF_ACTIVITY_MANAGER_HPP
#define DEF_ACTIVITY_MANAGER_HPP

#include "../Audio/MixingBus.h"
#include "../UserInterface/ArrangementArea.h"

class ActivityManager {
public:
    ActivityManager();
    ~Activitymanager();
    // TODO: add actions here eg moveSampleToPosition, etc...

private:
    std::vector<Action> history;
    UserInterfaceState uiState;
    std::vector<SampleState> samplesStates;
    int numTracks; // note: matches number of groups
    TaxonomyManager taxonomy;
};

#endif // DEF_ACTIVITY_MANAGER_HPP
