#include "AppState.h"

#include "ActivityManager.h"
#include "Marshalable.h"
#include "Task.h"
#include "TaxonomyManager.h"
#include "UserInterfaceState.h"
#include <memory>

AppState::AppState(ActivityManager &am) : activityManager(am)
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

std::optional<juce::File> AppState::getRepoDirectory()
{
    return repositoryFolder;
}

bool AppState::taskHandler(std::shared_ptr<Task> task)
{

    auto resetTask = std::dynamic_pointer_cast<ResetTask>(task);
    if (resetTask != nullptr && !resetTask->isCompleted())
    {
        git.setWorkingDirectory("");
        repositoryFolder.reset();
        resetTask->markStepDoneAndCheckCompletion();
        return false;
    }

    auto initGitTask = std::dynamic_pointer_cast<GitRepoInitTask>(task);
    if (initGitTask != nullptr && !initGitTask->isCompleted())
    {
        auto errMsg = initializeRepository(initGitTask->name);

        if (errMsg != "")
        {
            auto notifTask = std::make_shared<NotificationTask>(std::string(errMsg));
            activityManager.broadcastNestedTaskNow(notifTask);

            initGitTask->setFailed(true);
            return true;
        }

        // save existing state
        // initialize git in there
        // mark task as complete and rebroadcast it
        initGitTask->setCompleted(true);
        activityManager.broadcastNestedTaskNow(initGitTask);

        // notify user of the sucess :)
        auto notifTask = std::make_shared<NotificationTask>(
            std::string("The project repository was sucessfully initialized! \nMake sure to "
                        "commit changes as often as possible from the versionning menu."));
        activityManager.broadcastNestedTaskNow(notifTask);

        return true;
    }

    return false;
}

std::string AppState::initializeRepository(std::string name)
{
    // abort task if project already initialized
    if (repositoryFolder.has_value())
    {
        return "This project was already initialized.";
    }
    // abort if the folder already exists
    if (sharedConfig->isInvalid())
    {
        return "Config was not initialized.";
    }

    // ensure Data folder exists
    juce::File dataFolder(sharedConfig->getDataFolderPath());
    if (!dataFolder.exists() || !dataFolder.isDirectory())
    {
        return "Data folder missing or invalid: " + sharedConfig->getDataFolderPath();
    }

    // ensure Projects folder exists
    juce::File projectsFolder(dataFolder.getFullPathName() + "/Projects");
    if (projectsFolder.exists() && !projectsFolder.isDirectory())
    {
        return "Projects folder is not a directory!";
    }

    // if it doesn't exists, create it
    if (!projectsFolder.exists())
    {
        auto result = projectsFolder.createDirectory();
        if (result.failed())
        {
            return "Unable to create Projects folder: " + result.getErrorMessage().toStdString();
        }
    }

    // now if a repo with this project name already exists, abort
    juce::File newRepoFolder(projectsFolder.getFullPathName().toStdString() + "/" + name);
    if (newRepoFolder.exists())
    {
        return "A repository with that name already exists";
    }

    auto result = newRepoFolder.createDirectory();
    if (result.failed())
    {
        return "Failed to create project folder: " + result.getErrorMessage().toStdString();
    }

    repositoryFolder = newRepoFolder;

    dumpProjectFiles();
    git.setWorkingDirectory(repositoryFolder->getFullPathName().toStdString());
    git.init();
    git.add("main.json");
    git.add("samples.json");
    git.commit("initial commit");

    return "";
}

void AppState::dumpProjectFiles()
{
    if (!repositoryFolder.has_value())
    {
        return;
    }

    // Dump main.json
    std::string settingsJSON = marshal();
    juce::File mainJsonFile = repositoryFolder->getFullPathName() + "/main.json";
    mainJsonFile.create();
    mainJsonFile.replaceWithText(settingsJSON);

    // TODO: Dump samples.json
    std::string samplesJSON = "";
    juce::File samplesJsonFile = repositoryFolder->getFullPathName() + "/samples.json";
    samplesJsonFile.create();
    samplesJsonFile.replaceWithText(samplesJSON);
}