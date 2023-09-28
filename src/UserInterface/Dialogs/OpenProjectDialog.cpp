#include "OpenProjectDialog.h"
#include <ctime>
#include <exception>
#include <iomanip>
#include <memory>
#include <stdexcept>
#include <vector>

#include "../../Config.h"

#define PROJECT_TABLE_BUFFER_SIZE 50

#define DIALOG_OPEN_PROJECT_WIDTH 1000
#define DIALOG_OPEN_PROJECT_HEIGHT 500

OpenProjectDialog::OpenProjectDialog(ActivityManager &am)
    : activityManager(am), rowManager(sharedConfig.get().getDataFolderPath() + "/Projects"),
      projectsTable("Projects", TableSelectionMode::TABLE_SELECTION_ONE, rowManager, PROJECT_TABLE_BUFFER_SIZE)
{

    addAndMakeVisible(closeButton);
    closeButton.setButtonText("Cancel");

    addAndMakeVisible(confirmButton);
    confirmButton.setButtonText("Confirm");
    confirmButton.setEnabled(false);

    closeButton.addListener(this);
    confirmButton.addListener(this);

    addAndMakeVisible(projectsTable);

    projectsTable.addSelectionListener(this);

    setSize(DIALOG_OPEN_PROJECT_WIDTH, DIALOG_OPEN_PROJECT_HEIGHT);
}

void OpenProjectDialog::paint(juce::Graphics &g)
{
    g.setColour(COLOR_BACKGROUND);
    g.fillAll();
}

void OpenProjectDialog::receiveSelectionUpdate(std::set<int> selectedRowIndexes)
{
    if (selectedRowIndexes.size() == 1)
    {
        int displayedRowId = *selectedRowIndexes.begin();
        try
        {
            selectedFolder = rowManager.getFolderNameForRowIndex(displayedRowId);
            confirmButton.setEnabled(true);
            std::cout << "Selected folder " << selectedFolder << std::endl;
        }
        catch (std::runtime_error err)
        {
            std::cerr << "Error, received invalid selected row for project folder table" << std::endl;
            selectedFolder = "";
            confirmButton.setEnabled(false);
        }
    }
    else
    {
        selectedFolder = "";
        confirmButton.setEnabled(false);
    }
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

    if (button == &confirmButton)
    {
        std::string fullProjectPath = sharedConfig->getDataFolderPath() + "/Projects/" + selectedFolder;
        std::cout << "About to open project at path: " << fullProjectPath << std::endl;

        try
        {
            auto loadProjectTask = std::make_shared<OpenProjectTask>(fullProjectPath);
            activityManager.broadcastTask(loadProjectTask);
        }
        catch (std::exception &err)
        {
            std::string errMsg = std::string() + "Unable to open project: " + err.what();
            std::cerr << errMsg << std::endl;
            auto notifTask = std::make_shared<NotificationTask>("Unable to open project, see logs for more infos.",
                                                                ERROR_NOTIF_TYPE);
            activityManager.broadcastTask(notifTask);
        }
    }
}

///////////////////////////////////////////////////////

ProjectsDataFrame::ProjectsDataFrame(std::string projectFolderPath) : fullProjectsFolderPath(projectFolderPath)
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
            std::pair<TableType, TableColumnAlignment>(TableType::TABLE_COLUMN_TYPE_TEXT, TABLE_COLUMN_ALIGN_LEFT));
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

int ProjectsDataFrame::getMaxRowIndex()
{
    return projectsFoldersNames.size() - 1;
}

std::string ProjectsDataFrame::getFolderNameForRowIndex(int i)
{
    if (i < 0 || i >= orderedIds.size())
    {
        throw std::runtime_error("Tried to access a data frame row out of range!");
    }

    // translate this index to its ordering
    int dataFrameIndex = orderedIds[i];

    return projectsFoldersNames[dataFrameIndex];
}

DataframeRow ProjectsDataFrame::getRow(int n)
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
    rowsCache.insert(std::pair<int, DataframeRow>(dataFrameIndex, std::vector<std::shared_ptr<TableCell>>({})));
    auto row = rowsCache.find(dataFrameIndex);

    row->second.cells.reserve(7);

    row->second.cells.emplace_back(std::make_shared<TableCell>(projectsFoldersNames[dataFrameIndex],
                                                               TableColumnAlignment::TABLE_COLUMN_ALIGN_LEFT));
    row->second.cells.emplace_back(std::make_shared<TableCell>(
        formatDatetime(projectsFoldersCreatedTimeSec[dataFrameIndex]), TableColumnAlignment::TABLE_COLUMN_ALIGN_RIGHT));
    row->second.cells.emplace_back(
        std::make_shared<TableCell>(formatDatetime(projectsFoldersLastModifiedTimeSec[dataFrameIndex]),
                                    TableColumnAlignment::TABLE_COLUMN_ALIGN_RIGHT));

    // get git stats
    RepositoryStatistics repoStats;

    try
    {
        repoStats = git.getRepositoryStatistics(fullProjectsFolderPath + "/" + projectsFoldersNames[dataFrameIndex]);

        row->second.cells.emplace_back(
            std::make_shared<TableCell>(formatDatetime(repoStats.firstCommitDate),
                                        TableColumnAlignment::TABLE_COLUMN_ALIGN_CENTER)); // first commit date

        row->second.cells.emplace_back(
            std::make_shared<TableCell>(formatDatetime(repoStats.lastCommitDate),
                                        TableColumnAlignment::TABLE_COLUMN_ALIGN_CENTER)); // last commit date

        row->second.cells.emplace_back(std::make_shared<TableCell>(
            repoStats.noCommits, TableColumnAlignment::TABLE_COLUMN_ALIGN_CENTER)); // # of commits

        row->second.cells.emplace_back(std::make_shared<TableCell>(
            repoStats.noBranches, TableColumnAlignment::TABLE_COLUMN_ALIGN_CENTER)); // # of branches
    }
    catch (std::runtime_error err)
    {
        std::cerr << "Unable to get stats for a project in Projects folder: " << err.what() << std::endl;
        row->second.greyedOut = true;

        row->second.cells.emplace_back(
            std::make_shared<TableCell>("...",
                                        TableColumnAlignment::TABLE_COLUMN_ALIGN_CENTER)); // first commit date

        row->second.cells.emplace_back(
            std::make_shared<TableCell>("...",
                                        TableColumnAlignment::TABLE_COLUMN_ALIGN_CENTER)); // last commit date

        row->second.cells.emplace_back(
            std::make_shared<TableCell>(0, TableColumnAlignment::TABLE_COLUMN_ALIGN_CENTER)); // # of commits

        row->second.cells.emplace_back(
            std::make_shared<TableCell>(0, TableColumnAlignment::TABLE_COLUMN_ALIGN_CENTER)); // # of branches
    }

    // finally return it
    return row->second;
}

std::string ProjectsDataFrame::formatDatetime(std::time_t &t)
{
    // interesting discussion on formatting a date in C++:
    // https://stackoverflow.com/questions/16357999/current-date-and-time-as-string

    auto createLocalTime = *std::localtime(&t);
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

std::vector<int> ProjectsDataFrame::getColumnsWidth(int totalWidth)
{
    // temporary measure, in the future, we want to do better
    // TODO: enhance that algo

    int cellWidth = totalWidth / 7;

    std::vector<int> columnWidth(7);
    for (int i = 0; i < 7; i++)
    {
        columnWidth[i] = cellWidth;
    }

    return columnWidth;
}