#include "AppState.h"

#include "Marshalable.h"
#include "SampleState.h"
#include "TaxonomyManager.h"
#include "UserInterfaceState.h"

AppState::AppState()
{
    uiState = UI_STATE_DEFAULT;
}
AppState::~AppState()
{
}

void setSample(int id, SampleState *)
{
    // TODO
}

TaxonomyManager &AppState::getTaxonomy()
{
    return taxonomy;
}

std::string AppState::Marshal()
{
    // TODO
    return "none";
}

Marshalable *AppState::Unmarshal(std::string &s)
{
    return nullptr;
}

UserInterfaceState &AppState::getUiState()
{
    return uiState;
}

void AppState::setUiState(UserInterfaceState s)
{
    uiState = s;
}