#include "OpenProjectDialog.h"

OpenProjectDialog::OpenProjectDialog(ActivityManager &am) : activityManager(am)
{
}

void OpenProjectDialog::paint(juce::Graphics &g)
{
}

void OpenProjectDialog::resized()
{
}

void OpenProjectDialog::closeDialog()
{
}

void OpenProjectDialog::buttonClicked(juce::Button *button)
{
}

///////////////////////////////////////////////////////

ProjectsDataFrame::ProjectsDataFrame(std::string projectFolderPath)
{

    // first we're loading all the subfolder of the project folder

    juce::File projectsFolderFile(projectFolderPath);

    auto subFolders = projectsFolderFile.findChildFiles(juce::File::TypesOfFileToFind::findDirectories, false, "*",
                                                        juce::File::FollowSymlinks::no);

    projectsFoldersNames.reserve(subFolders.size());
    projectsFoldersLastModifiedTimeSec.reserve(subFolders.size());
    projectsFoldersCreatedTimeSec.reserve(subFolders.size());

    for (int i = 0; i < subFolders.size(); i++)
    {
        // we collect folder name and timestamp for creation and last modification
        projectsFoldersNames.push_back(subFolders[i].getFileName().toStdString());
        projectsFoldersLastModifiedTimeSec.push_back(subFolders[i].getLastModificationTime().toMilliseconds() / 1000);
        projectsFoldersCreatedTimeSec.push_back(subFolders[i].getCreationTime().toMilliseconds() / 1000);
    }

    // we define the column names
    colNames = {"Name",        "Created",           "Last modified",     "First commit",
                "Last commit", "Number of commits", "Number of branches"};

    // we then proceed to define the (static) format of the dataframe
    format.reserve(7);
    // TODO

    // we then set the ordering (default to descending creation date)
    // TODO
}

std::vector<std::string> &ProjectsDataFrame::getColumnNames()
{
    return colNames;
}

int ProjectsDataFrame::getMaxRowNumber()
{
    return projectsFoldersNames.size();
}

std::vector<TableCell> &ProjectsDataFrame::getRow(int n)
{
}

std::vector<std::pair<TableType, TableColumnAlignment>> &ProjectsDataFrame::getFormat()
{
}

std::optional<std::pair<int, bool>> &ProjectsDataFrame::getOrdering()
{
    return ordering;
}

bool ProjectsDataFrame::trySettingOrdering(std::pair<int, bool> ordering)
{
    return false;
}
