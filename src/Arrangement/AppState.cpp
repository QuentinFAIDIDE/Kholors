#include "AppState.h"

#include "ActivityManager.h"
#include "Marshalable.h"
#include "NumericInputId.h"
#include "Task.h"
#include "TaxonomyManager.h"
#include "UserInterfaceState.h"
#include <memory>

#include <stdexcept>
#define APP_STATE_API_VERSION "v0"

AppState::AppState(ActivityManager &am) : activityManager(am)
{
    uiState = UI_STATE_DEFAULT;

    taxonomyStateFileHandler = &taxonomy;
    mixbusStateFileHandler = nullptr;
    uiStateFileHandler = nullptr;
}

AppState::~AppState()
{
}

void AppState::setExternalAppStateFileHandlers(Marshalable *uiStateHandler, Marshalable *mixbusStateHandler)
{
    uiStateFileHandler = uiStateHandler;
    mixbusStateFileHandler = mixbusStateHandler;
}

TaxonomyManager &AppState::getTaxonomy()
{
    return taxonomy;
}

std::string AppState::marshal()
{
    json appjson = {
        {"api_version", APP_STATE_API_VERSION},
        {"tempo", tempo},
    };
    return appjson.dump();
}

void AppState::unmarshal(std::string &s)
{
    json appjson = json::parse(s);

    if (appjson.find("api_version") == appjson.end() || !appjson["api_version"].is_string())
    {
        throw std::runtime_error("Undefined api_version or wrong format in app json");
    }

    if (appjson["api_version"] != APP_STATE_API_VERSION)
    {
        throw std::runtime_error("Unsupported app file api version");
    }

    if (appjson.find("tempo") == appjson.end() || !appjson["tempo"].is_number())
    {
        throw std::runtime_error("Undefined tempo or wrong format in app json");
    }

    tempo = (float)appjson["tempo"];
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
        tempo = DEFAULT_TEMPO;
        resetTask->markStepDoneAndCheckCompletion();
        return false;
    }

    auto initGitTask = std::dynamic_pointer_cast<GitRepoInitTask>(task);
    if (initGitTask != nullptr && !initGitTask->isCompleted())
    {
        try
        {
            initializeRepository(initGitTask->name);
        }
        catch (std::runtime_error err)
        {
            auto notifTask = std::make_shared<NotificationTask>(std::string(err.what()));
            activityManager.broadcastNestedTaskNow(notifTask);

            std::cerr << "Error while trying to initialize project: " << err.what() << std::endl;

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

    // mirror tempo updates here
    auto tempoUpdate = std::dynamic_pointer_cast<NumericInputUpdateTask>(task);
    if (tempoUpdate != nullptr && tempoUpdate->numericalInputId == NUM_INPUT_ID_TEMPO && tempoUpdate->isCompleted())
    {
        tempo = tempoUpdate->newValue;
        std::cout << "tempo was updated to " << tempo << std::endl;
        return false;
    }

    return false;
}

void AppState::initializeRepository(std::string name)
{
    // abort task if project already initialized
    if (repositoryFolder.has_value())
    {
        throw std::runtime_error("unable to initiliaze a project that was already initialized");
    }
    // abort if the folder already exists
    if (sharedConfig->isInvalid())
    {
        throw std::runtime_error("unable to initialize repository folder because config object was not initialized.");
    }

    // ensure Data folder exists
    juce::File dataFolder(sharedConfig->getDataFolderPath());
    if (!dataFolder.exists() || !dataFolder.isDirectory())
    {
        throw std::runtime_error(std::string("Data folder missing or invalid: ") + sharedConfig->getDataFolderPath());
    }

    // ensure Projects folder exists
    juce::File projectsFolder(dataFolder.getFullPathName() + "/Projects");
    if (projectsFolder.exists() && !projectsFolder.isDirectory())
    {
        throw std::runtime_error("Projects folder to save in is not a directory!");
    }

    // if it doesn't exists, create it
    if (!projectsFolder.exists())
    {
        auto result = projectsFolder.createDirectory();
        if (result.failed())
        {
            throw std::runtime_error("Unable to create Projects folder: " + result.getErrorMessage().toStdString());
        }
    }

    // now if a repo with this project name already exists, abort
    juce::File newRepoFolder(projectsFolder.getFullPathName().toStdString() + "/" + name);
    if (newRepoFolder.exists())
    {
        throw std::runtime_error("A repository with that name already exists");
    }

    auto result = newRepoFolder.createDirectory();
    if (result.failed())
    {
        throw std::runtime_error("Failed to create project folder: " + result.getErrorMessage().toStdString());
    }

    repositoryFolder = newRepoFolder;

    dumpProjectFiles();

    git.setWorkingDirectory(repositoryFolder->getFullPathName().toStdString());

    git.init();

    git.add("app.json");
    git.add("taxonomy.json");
    git.add("mixbus.json");
    git.add("ui.json");

    git.commit("initial commit");
}

void AppState::dumpProjectFiles()
{
    if (!repositoryFolder.has_value())
    {
        throw std::runtime_error("unable to dump project files since no project folder has been set");
    }

    std::string settingsJSON = marshal();
    juce::File mainJsonFile = repositoryFolder->getFullPathName() + "/app.json";
    mainJsonFile.create();
    mainJsonFile.replaceWithText(settingsJSON);

    std::string taxonomyJSON = taxonomyStateFileHandler->marshal();
    juce::File mainTaxonomyFile = repositoryFolder->getFullPathName() + "/taxonomy.json";
    mainTaxonomyFile.create();
    mainTaxonomyFile.replaceWithText(taxonomyJSON);

    std::string samplesJSON = mixbusStateFileHandler->marshal();
    juce::File samplesJsonFile = repositoryFolder->getFullPathName() + "/mixbus.json";
    samplesJsonFile.create();
    samplesJsonFile.replaceWithText(samplesJSON);

    std::string uiJSON = uiStateFileHandler->marshal();
    juce::File uiJsonFile = repositoryFolder->getFullPathName() + "/ui.json";
    uiJsonFile.create();
    uiJsonFile.replaceWithText(uiJSON);
}