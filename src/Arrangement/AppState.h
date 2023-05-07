#ifndef DEF_APPSTATE_HPP
#define DEF_APPSTATE_HPP

#include "Marshalable.h"
#include "SampleState.h"
#include "TaxonomyManager.h"
#include "UserInterfaceState.h"

class AppState : public Marshalable
{
  public:
    AppState();
    ~AppState();
    TaxonomyManager &getTaxonomy();
    std::string marshal() override final;
    Marshalable *unmarshal(std::string &) override final;
    UserInterfaceState &getUiState();
    void setUiState(UserInterfaceState);

  private:
    std::vector<SampleState *> sampleStates;
    TaxonomyManager taxonomy;
    UserInterfaceState uiState;
};

#endif