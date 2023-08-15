#ifndef DEF_APPSTATE_HPP
#define DEF_APPSTATE_HPP

#include "Marshalable.h"
#include "TaskListener.h"
#include "TaxonomyManager.h"
#include "UserInterfaceState.h"
#include <optional>

class ActivityManager;

/**
AppState is responsible for states, ie
things like taxonomy, sample, colors.
It is Marshalable in the sense that it
can be saved to a json file and be reloaded
later.
*/
class AppState : public Marshalable, public TaskListener
{
  public:
    /**
    Constructor to initialize the app state manager.
    */
    AppState(ActivityManager &am);
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

    /**
     * @brief      Gets the repo directory optional container.
     *             If not present, repo is unitialized.
     *
     * @return     The repo directory if exists.
     */
    std::optional<juce::File> getRepoDirectory();

    /**
     * @brief      Parse new tasks and eventually process them if they
     *             are deemed interesting.
     *
     * @param[in]  task  The task
     *
     * @return     true if broadcast of this task should go no further in the TaskListeners list, false
     *             if it should.
     */
    bool taskHandler(std::shared_ptr<Task> task) override;

    /**
     * @brief      Initializes a repository with that name and
     *             return error msg. If everything went ok, errMsg
     *             is empty.
     */
    std::string initializeRepository(std::string name);

  private:
    TaxonomyManager taxonomy;
    UserInterfaceState uiState;
    std::optional<juce::File> repositoryFolder;
    ActivityManager &activityManager;
    juce::SharedResourcePointer<Config> sharedConfig;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AppState)
};

#endif