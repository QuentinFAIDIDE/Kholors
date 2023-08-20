#include "GitInitRepoDialog.h"
#include <memory>
#include <regex>

#include "../../Config.h"
#include "../Section.h"
#include "LineEntryDialog.h"

GitInitRepoDialog::GitInitRepoDialog(ActivityManager &am) : LineEntryDialog(am)
{
    initializeDialog();
}

std::regex GitInitRepoDialog::getEntryRegex()
{
    return std::regex(REPO_NAME_VALIDATION_REGEX);
}

std::string GitInitRepoDialog::getTextEntryDescription()
{
    return std::string("Enter your new project name");
}

int GitInitRepoDialog::getDialogWidth()
{
    return GIT_INIT_REPO_DIALOG_WIDTH;
}

int GitInitRepoDialog::getDialogHeight()
{
    return GIT_INIT_REPO_DIALOG_HEIGHT;
}

std::string GitInitRepoDialog::getDialogInstructions()
{
    return std::string("Pick a name for the new repository");
}

void GitInitRepoDialog::performDialogTask(std::string dialogEntry)
{
    auto trackInitTask = std::make_shared<GitRepoInitTask>(dialogEntry);
    getActivityManager().broadcastTask(trackInitTask);
}