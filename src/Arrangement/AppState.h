#ifndef DEF_APPSTATE_HPP
#define DEF_APPSTATE_HPP

#include "GitWrapper.h"
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
    void unmarshal(std::string &) override final;

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
     * @brief      Initializes a repository with that name.
     */
    void initializeRepository(std::string name);

    /**
     * @brief      Dumps project files into the json and settings in the struct.
     */
    void dumpProjectFiles();

    /**
     * @brief      Sets the external application state file handlers. These are Marshalable
     *             that can dump and load jsons to restore software states.
     *             Commonly set in the desktop app to the ArrangemeentArea for ui, and MixbingBus
     *             for mixbus state handler.
     *
     * @param      uiStateHandler      The state handler
     * @param      mixbusStateHandler  The mixbus state handler
     */
    void setExternalAppStateFileHandlers(Marshalable *uiStateHandler, Marshalable *mixbusStateHandler);

  private:
    TaxonomyManager taxonomy;
    UserInterfaceState uiState;
    ActivityManager &activityManager;
    juce::SharedResourcePointer<Config> sharedConfig;
    GitWrapper git;
    float tempo;

    Marshalable *uiStateFileHandler;
    Marshalable *mixbusStateFileHandler;
    Marshalable *taxonomyStateFileHandler;

    std::optional<juce::File> repositoryFolder;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AppState)
};

#endif