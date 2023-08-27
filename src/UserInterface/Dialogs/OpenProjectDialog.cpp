#include "OpenProjectDialog.h"
#include <ctime>
#include <iomanip>
#include <stdexcept>
#include <vector>

#include "../../Config.h"

OpenProjectDialog::OpenProjectDialog(ActivityManager &am)
    : activityManager(am), rowManager(sharedConfig.get().getDataFolderPath() + "/Projects"),
      projectsTable("Projects", TableSelectionMode::TABLE_SELECTION_NONE, rowManager)
{

    addAndMakeVisible(closeButton);
    closeButton.setButtonText("Cancel");

    addAndMakeVisible(confirmButton);
    confirmButton.setButtonText("Confirm");
    confirmButton.setEnabled(false);

    closeButton.addListener(this);
    confirmButton.addListener(this);

    addAndMakeVisible(projectsTable);
}

void OpenProjectDialog::paint(juce::Graphics &g)
{
    g.setColour(COLOR_BACKGROUND);
    g.fillAll();
}

void OpenProjectDialog::resized()
{
    auto bounds = getLocalBounds();
    auto buttonArea = bounds.removeFromBottom(DIALOG_FOOTER_AREA_HEIGHT);

    // remove side margins and center vertically for buttons
    buttonArea.reduce(DIALOG_FOOTER_BUTTONS_SPACING, (DIALOG_FOOTER_AREA_HEIGHT - DIALOG_FOOTER_BUTTONS_HEIGHT) / 2);

    // position buttons
    confirmButton.setBounds(buttonArea.removeFromRight(DIALOG_FOOTER_BUTTONS_WIDTH));
    buttonArea.removeFromRight(DIALOG_FOOTER_BUTTONS_SPACING);
    closeButton.setBounds(buttonArea.removeFromRight(DIALOG_FOOTER_BUTTONS_WIDTH));

    // position table
    projectsTable.setBounds(bounds);
}

void OpenProjectDialog::closeDialog()
{
    if (juce::DialogWindow *dw = findParentComponentOfClass<juce::DialogWindow>())
        dw->exitModalState(0);
}

void OpenProjectDialog::buttonClicked(juce::Button *button)
{
    if (button == &closeButton || button == &confirmButton)
    {
        closeDialog();
    }
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

    // and from that compute the header format
    for (int i = 0; i < format.size(); i++)
    {
        headerFormat.push_back(
            std::pair<TableType, TableColumnAlignment>(TableType::TABLE_COLUMN_TYPE_TEXT, format[i].second));
    }

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
    if (n < 0 || n >= orderedIds.size())
    {
        throw std::runtime_error("Tried to access a data frame row out of range!");
    }

    // translate this index to its ordering
    int dataFrameIndex = orderedIds[n];

    // if it's in cache return it directly
    auto rowInCache = rowsCache.find(dataFrameIndex);
    if (rowInCache != rowsCache.end())
    {
        return rowInCache->second;
    }

    // if it's not in cache first build it
    rowsCache[dataFrameIndex] = std::vector<TableCell>();

    rowsCache[dataFrameIndex].push_back(TableCell(projectsFoldersNames[dataFrameIndex]));
    rowsCache[dataFrameIndex].push_back(TableCell(formatDatetime(projectsFoldersCreatedTimeSec[dataFrameIndex])));
    rowsCache[dataFrameIndex].push_back(TableCell(formatDatetime(projectsFoldersLastModifiedTimeSec[dataFrameIndex])));

    // TODO: add git integration
    rowsCache[dataFrameIndex].push_back(TableCell("")); // first commit date
    rowsCache[dataFrameIndex].push_back(TableCell("")); // last commit date
    rowsCache[dataFrameIndex].push_back(TableCell(0));  // # of commits
    rowsCache[dataFrameIndex].push_back(TableCell(0));  // # of branches

    // finally return it
    return rowsCache[dataFrameIndex];
}

std::string ProjectsDataFrame::formatDatetime(std::time_t &t)
{
    // interesting discussion on formatting a date in C++:
    // https://stackoverflow.com/questions/16357999/current-date-and-time-as-string

    auto createLocalTime = *std::localtime(&projectsFoldersCreatedTimeSec[t]);
    std::ostringstream oss;
    oss << std::put_time(&createLocalTime, DATETIME_FORMAT_1);
    return oss.str();
}

std::vector<std::pair<TableType, TableColumnAlignment>> &ProjectsDataFrame::getFormat()
{
    return format;
}

std::vector<std::pair<TableType, TableColumnAlignment>> &ProjectsDataFrame::getHeaderFormat()
{
    return headerFormat;
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
