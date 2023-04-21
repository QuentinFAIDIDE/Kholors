#ifndef DEF_APPSTATE_HPP
#define DEF_APPSTATE_HPP

#include "UserInterfaceState.h"
#include "Marshalable.h"

class AppState: public Marshalable {
public:
    AppState();
    ~AppState();
    void setSample(int id, SampleState*);
    SampleState* getSample(int id);
    Taxonomy& getTaxonomy();
    std::string Marshal();
    Marshalable* Unmarshal();
    UserInterfaceState& getUiState();
    void setUiState(UserInterfaceState);

private:
    std::vector<SampleState*> sampleStates;
    Taxonomy taxonomy;
    UserInterfaceState uiState;
};

#endif