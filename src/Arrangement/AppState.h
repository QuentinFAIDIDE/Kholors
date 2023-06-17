#ifndef DEF_APPSTATE_HPP
#define DEF_APPSTATE_HPP

#include "Marshalable.h"
#include "TaxonomyManager.h"
#include "UserInterfaceState.h"

/**
AppState is responsible for states, ie
things like taxonomy, sample, colors.
It is Marshalable in the sense that it
can be saved to a json file and be reloaded
later.
*/
class AppState : public Marshalable
{
  public:
    /**
    Constructor to initialize the app state manager.
    */
    AppState();
    ~AppState();

    /**
    Returns a reference to a taxonomy manager
    */
    TaxonomyManager &getTaxonomy();

    /**
    Persist this struct to text (as of right now, JSON).
    */
    std::string marshal() override final;

    /**
    Initialize this struct from another text.
    */
    Marshalable *unmarshal(std::string &) override final;

    /**
    Get the state where the User Interface is at. The state
    can be Resizing, Selecting, SHrinking, etc...
    */
    UserInterfaceState &getUiState();

    /**
    Sets the state the interface is in
    */
    void setUiState(UserInterfaceState);

  private:
    TaxonomyManager taxonomy;
    UserInterfaceState uiState;
};

#endif