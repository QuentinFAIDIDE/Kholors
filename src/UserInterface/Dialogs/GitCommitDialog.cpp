#include "GitCommitDialog.h"
#include <memory>
#include <regex>
#include <string>

#include "../../Config.h"
#include "../Section.h"
#include "LineEntryDialog.h"

GitCommitDialog::GitCommitDialog(ActivityManager &am) : LineEntryDialog(am)
{
    initializeDialog();
}

std::regex GitCommitDialog::getEntryRegex()
{
    return std::regex(GIT_COMMIT_REGEX);
}

std::string GitCommitDialog::getTextEntryDescription()
{
    return std::string("Enter a commit message for your changes");
}

int GitCommitDialog::getDialogWidth()
{
    return GIT_COMMIT_DIALOG_WIDTH;
}

int GitCommitDialog::getDialogHeight()
{
    return GIT_COMMIT_DIALOG_HEIGHT;
}

std::string GitCommitDialog::getDialogInstructions()
{
    return std::string("Describe your changes in a short message");
}

void GitCommitDialog::performDialogTask(std::string dialogEntry)
{
    auto commitTask = std::make_shared<GitCommitTask>(dialogEntry);
    getActivityManager().broadcastTask(commitTask);
}