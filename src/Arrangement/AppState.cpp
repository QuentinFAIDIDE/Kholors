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
    return appjson.dump(JSON_STATE_SAVING_INDENTATION);
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
    auto tempoTask = std::make_shared<NumericInputUpdateTask>(NUM_INPUT_ID_TEMPO, tempo);
    activityManager.broadcastNestedTaskNow(tempoTask);

    if (tempoTask->hasFailed())
    {
        throw std::runtime_error("unable to update tempo");
    }
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
            auto notifTask = std::make_shared<NotificationTask>(err.what(), ERROR_NOTIF_TYPE);
            activityManager.broadcastNestedTaskNow(notifTask);

            std::cerr << "Error while trying to initialize project: " << err.what() << std::endl;

            initGitTask->setFailed(true);
            return true;
        }

        // mark task as complete and rebroadcast it in case some ui component that comes
        // before in the taskListeners queue need to update state
        initGitTask->setCompleted(true);
        activityManager.broadcastNestedTaskNow(initGitTask);

        // notify user of the sucess :)
        auto notifTask =
            std::make_shared<NotificationTask>("The project repository was sucessfully initialized! \nMake sure to "
                                               "commit changes as often as possible from the versionning menu.",
                                               INFO_NOTIF_TYPE);
        activityManager.broadcastNestedTaskNow(notifTask);

        return true;
    }

    auto commitTask = std::dynamic_pointer_cast<GitCommitTask>(task);
    if (commitTask != nullptr && !commitTask->isCompleted())
    {
        try
        {
            dumpProjectFiles();
            git.add("app.json");
            git.add("taxonomy.json");
            git.add("mixbus.json");
            git.add("ui.json");
            git.commit(commitTask->message);

            activityManager.clearTaskHistory();
        }
        catch (std::runtime_error err)
        {
            auto notifTask = std::make_shared<NotificationTask>(err.what(), ERROR_NOTIF_TYPE);
            activityManager.broadcastNestedTaskNow(notifTask);

            std::cerr << "Error while trying to initialize project: " << err.what() << std::endl;

            commitTask->setFailed(true);
            return true;
        }

        // rebroadcasted as completed for ui components that are listeners and are before this
        // component in the task listeners list to update if required
        commitTask->setCompleted(true);
        activityManager.broadcastNestedTaskNow(commitTask);

        // notify user of the sucess :)
        auto notifTask =
            std::make_shared<NotificationTask>("Your changes were successfully committed.", INFO_NOTIF_TYPE);
        activityManager.broadcastNestedTaskNow(notifTask);

        return true;
    }

    // mirror tempo updates here
    auto tempoUpdate = std::dynamic_pointer_cast<NumericInputUpdateTask>(task);
    if (tempoUpdate != nullptr && tempoUpdate->numericalInputId == NUM_INPUT_ID_TEMPO && tempoUpdate->isCompleted())
    {
        tempo = tempoUpdate->newValue;
        return false;
    }

    auto projectOpeningTask = std::dynamic_pointer_cast<OpenProjectTask>(task);
    if (projectOpeningTask != nullptr && projectOpeningTask->stage == OPEN_PROJECT_STAGE_APP_STATE_SETUP)
    {
        try
        {
            // this will prevent the sample from getting unload untill the new project is loaded
            // which helps at reusing loaded files.
            sharedAudioFileBuffers->disableUnusedBuffersRelease();
            textureManager->lockCurrentTextures();

            auto clearTask = std::make_shared<ResetTask>();
            activityManager.broadcastNestedTaskNow(clearTask);

            if (clearTask->hasFailed())
            {
                throw std::runtime_error("unable to clear app state");
            }

            // when done broadcasting, make sure to open the project folder so that we ofically recognise it as open and
            // can commit
            repositoryFolder = juce::File(projectOpeningTask->projectFolderPath);
            if (!repositoryFolder->isDirectory())
            {
                throw std::runtime_error("invalid project repository folder");
            }
            git.setWorkingDirectory(projectOpeningTask->projectFolderPath);
            git.open();

            unmarshal(projectOpeningTask->appStateConfig);

            projectOpeningTask->stage = OPEN_PROJECT_STAGE_TAXONOMY_SETUP;

            taxonomy.unmarshal(projectOpeningTask->taxonomyConfig);

            projectOpeningTask->stage = OPEN_PROJECT_STAGE_ARRANGEMENT_SETUP;
            // This will pass the task to the arrangement area that will broadcast it again in another
            // state so that it's picked by mixbus and then set as completed
            activityManager.broadcastNestedTaskNow(projectOpeningTask);

            // since the broadcastNestedTaskNow run the subtasks synchronously, we can consider
            // loading is finished here and reenable the audio buffer freeing
            sharedAudioFileBuffers->enableUnusedBuffersRelease();

            // we decrement usage count we increased earlier and free non reused textures
            textureManager->releaseLockedTextures();

            // and this will record the task in history (note that broadcastNestTaskNow unlike broadcastTask guarantees
            // execution)
            return true;
        }
        catch (std::exception &err)
        {
            // this need to be reenabled
            sharedAudioFileBuffers->enableUnusedBuffersRelease();
            textureManager->releaseLockedTextures();

            projectOpeningTask->setFailed(true);
            projectOpeningTask->stage = OPEN_PROJECT_STAGE_FAILED;

            std::cerr << "Unable to open project: " << err.what() << std::endl;

            auto notifTask = std::make_shared<NotificationTask>("Unable to open project, see logs for more infos.",
                                                                ERROR_NOTIF_TYPE);
            activityManager.broadcastNestedTaskNow(notifTask);

            return true;
        }
    }

    auto hardHeadResetTask = std::dynamic_pointer_cast<GitHeadResetTask>(task);
    if (hardHeadResetTask != nullptr && hardHeadResetTask->isCompleted() == false && !hardHeadResetTask->hasFailed())
    {
        try
        {

            if (!repositoryFolder.has_value())
            {
                throw std::runtime_error("No project repository folder currently set");
            }

            // perform the git reset
            git.resetCurrentChanges();

            // backup folder path
            juce::File repoToReset = repositoryFolder.value();

            // reload the project
            auto loadProjectTask = std::make_shared<OpenProjectTask>(repoToReset.getFullPathName().toStdString());
            activityManager.broadcastNestedTaskNow(loadProjectTask);
            if (loadProjectTask->hasFailed() || !loadProjectTask->isCompleted())
            {
                throw std::runtime_error("Unable to reload project");
            }
        }
        catch (std::exception &err)
        {
            std::cerr << "Failed to reset: " << err.what() << std::endl;
            hardHeadResetTask->setFailed(true);
            return true;
        }

        hardHeadResetTask->setCompleted(true);
        return true;
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