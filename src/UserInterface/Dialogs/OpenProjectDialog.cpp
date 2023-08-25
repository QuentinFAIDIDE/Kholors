#include "OpenProjectDialog.h"
#include <stdexcept>

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

    if (!projectsFolderFile.isDirectory())
    {
        throw std::runtime_error("Project folder passed to object is invalid");
    }

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
    format.push_back(std::pair<TableType, TableColumnAlignment>(TABLE_COLUMN_TYPE_TEXT, TABLE_COLUMN_ALIGN_LEFT));
    format.push_back(std::pair<TableType, TableColumnAlignment>(TABLE_COLUMN_TYPE_TEXT, TABLE_COLUMN_ALIGN_LEFT));
    format.push_back(std::pair<TableType, TableColumnAlignment>(TABLE_COLUMN_TYPE_TEXT, TABLE_COLUMN_ALIGN_LEFT));
    format.push_back(std::pair<TableType, TableColumnAlignment>(TABLE_COLUMN_TYPE_TEXT, TABLE_COLUMN_ALIGN_LEFT));
    format.push_back(std::pair<TableType, TableColumnAlignment>(TABLE_COLUMN_TYPE_TEXT, TABLE_COLUMN_ALIGN_LEFT));
    format.push_back(std::pair<TableType, TableColumnAlignment>(TABLE_COLUMN_TYPE_INT, TABLE_COLUMN_ALIGN_RIGHT));
    format.push_back(std::pair<TableType, TableColumnAlignment>(TABLE_COLUMN_TYPE_INT, TABLE_COLUMN_ALIGN_RIGHT));

    if (!trySettingOrdering(std::pair<int, bool>(1, false)))
    {
        throw std::runtime_error("default ordering from dataframe constructor was not implemented");
    }
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

bool ProjectsDataFrame::sortByCreationDate(int a, int b) const
{
    if (a >= projectsFoldersCreatedTimeSec.size() || b >= projectsFoldersCreatedTimeSec.size())
    {
        throw std::runtime_error("Sorting index above creation timestamp vector size!");
    }

    return projectsFoldersCreatedTimeSec[a] < projectsFoldersCreatedTimeSec[b];
}

bool ProjectsDataFrame::sortByLastEditDate(int a, int b) const
{
    if (a >= projectsFoldersLastModifiedTimeSec.size() || b >= projectsFoldersLastModifiedTimeSec.size())
    {
        throw std::runtime_error("Sorting index above last edit timestamp vector size!");
    }

    return projectsFoldersLastModifiedTimeSec[a] < projectsFoldersLastModifiedTimeSec[b];
}

bool ProjectsDataFrame::trySettingOrdering(std::pair<int, bool> desiredOrdering)
{

    // if ordering on folder creation or last edit date, we proceed
    if (desiredOrdering.first == 1 || desiredOrdering.first == 2)
    {
        orderedIds.resize(projectsFoldersNames.size());
        std::iota(orderedIds.begin(), orderedIds.end(), 0); // create sequential ids

        // trick to use non static member func for sorting
        auto sorter =
            std::bind(&ProjectsDataFrame::sortByCreationDate, this, std::placeholders::_1, std::placeholders::_2);
        if (desiredOrdering.first == 2)
        {
            std::bind(&ProjectsDataFrame::sortByLastEditDate, this, std::placeholders::_1, std::placeholders::_2);
        }

        // do the actual sorting
        std::sort(orderedIds.begin(), orderedIds.end(), sorter);

        // and reverse if user asked for descending
        if (!desiredOrdering.second)
        {
            std::reverse(orderedIds.begin(), orderedIds.end());
        }

        return true;
    }
    // if ordering on anything else, abort
    else
    {
        return false;
    }
}
