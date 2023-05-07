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

TaxonomyManager &AppState::getTaxonomy()
{
    return taxonomy;
}

std::string AppState::marshal()
{
    // TODO
    return "none";
}

Marshalable *AppState::unmarshal(std::string &s)
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