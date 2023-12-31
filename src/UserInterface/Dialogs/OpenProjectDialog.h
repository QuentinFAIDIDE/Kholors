#ifndef DEF_OPEN_PROJECT_DIALOG_HPP
#define DEF_OPEN_PROJECT_DIALOG_HPP

#include "../../Arrangement/ActivityManager.h"
#include "../Section.h"
#include "../Widgets/Table.h"

class ProjectsDataFrame : public TableDataFrame
{
  public:
    ProjectsDataFrame(std::string projectFolderPath);

    /**
     * @brief      Gets the maximum index of row this data loader delivers.
     *             We can assume at some point we will allow to return -1 to indicate it's infinite
     *             or not known.
     *
     * @return     The maximum row index.
     */
    int getMaxRowIndex() override;

    /**
     * @brief      Gets the row at index. Index as in table line index starting at 0.
     *
     * @param[in]  n     0 starting row number of the row.
     *
     * @return     The row.
     */
    DataframeRow getRow(int n) override;

    /**
     * @brief      Gets the format of the dataframe, as a vector of
     *             column types paired with their alignment.
     *
     * @return     The format of the dataframe
     */
    std::vector<std::pair<TableType, TableColumnAlignment>> &getFormat() override;

    /**
     * @brief      Gets the format of the dataframe header, it's a vector of string the size
     *             of the regular format basically.
     *
     * @return     The format of the dataframe
     */
    std::vector<std::pair<TableType, TableColumnAlignment>> &getHeaderFormat() override;

    /**
     * @brief      Gets the ordering, as a pair of column id and a boolean
     *             indicating if ascending. It can return nothing, meaning there are no ordering
     *             available.
     *
     * @return     The ordering.
     */
    std::optional<std::pair<int, bool>> &getOrdering() override;

    /**
     * @brief      Tried to set the specified ordering. If suceeding, the
     *             ids of the rows will be reset and you should clear the table content
     *             and query the rows again.
     *
     * @param[in]  ordering  The ordering as a pair a column id and boolean
     *             indicating if ascending.
     *
     * @return     true if suceeded, false if failed.
     */
    bool trySettingOrdering(std::pair<int, bool> ordering) override;

    /**
     * @brief      Gets the header as a list of column names.
     *
     * @return     The vector of string column names.
     */
    std::vector<std::string> &getColumnNames() override;

    /**
     * @brief      Gets the columns width for specific total width.
     *
     * @return     The columns width.
     */
    std::vector<int> getColumnsWidth(int totalWidth) override;

    /**
     * @brief      Gets the folder name for the row at that index.
     *
     * @param[in]  i     index of the row in the displayed data (starting at 0).
     *
     * @return     The folder name.
     */
    std::string getFolderNameForRowIndex(int i);

  private:
    std::vector<std::string> projectsFoldersNames; /**< projects folder names (in loading order) */
    std::vector<std::time_t>
        projectsFoldersLastModifiedTimeSec; /**< projects folder last edit timestamps (in loading order) */
    std::vector<std::time_t>
        projectsFoldersCreatedTimeSec;            /**< projects folder creation timestamps (in loading order) */
    std::map<int, DataframeRow> rowsCache;        /**< rows that are cached */
    std::optional<std::pair<int, bool>> ordering; /**< column index of ordering and bool to know if ascending */
    std::vector<std::pair<TableType, TableColumnAlignment>> format; /**< column types and alignments */
    std::vector<std::pair<TableType, TableColumnAlignment>>
        headerFormat;                  /**< column types and alignments for header row */
    std::vector<std::string> colNames; /**< header col names */
    std::vector<int> orderedIds;       /**<
                                        vector of indexes of projectsFoldersNames rows that are sorted in specified ordering.
                                       */
    GitWrapper git;                    /**< Object that load git stats about projects */
    std::string fullProjectsFolderPath;

    /**
     * @brief      Formats timestamp into appropriate time
     *
     * @param      t    Unix timestamp in seconds.
     *
     * @return     stringified time in configured format.
     */
    std::string formatDatetime(std::time_t &t);

    // helper to sort rows by creation date
    bool sortByCreationDate(int a, int b) const;

    // helper to sort rows by last edit date
    bool sortByLastEditDate(int a, int b) const;
};

class OpenProjectDialog : public juce::Component, juce::Button::Listener, TableSelectionListener
{
  public:
    /**
     * @brief      Constructs a new instance.
     *
     * @param      am    Reference to the app activity manager to post tasks.
     */
    OpenProjectDialog(ActivityManager &am);

    /**
     * @brief      Juce paint callback.
     *
     */
    void paint(juce::Graphics &g) override;

    /**
     * @brief      Juce positioning callback.
     */
    void resized() override;

    /**
     * @brief      Closes self by trying to reach parent juce::DialogWindow.
     */
    void closeDialog();

    /**
     * @brief      Called when a button is clicked.
     *
     * @param      button  The button
     */
    void buttonClicked(juce::Button *button) override;

    /**
     * @brief      Callback to receive something when selected rows changes.
     *             The int received are indices of the rows.
     *
     * @param[in]  selectedRowIndexes  The selected row indexes
     */
    void receiveSelectionUpdate(std::set<int> selectedRowIndexes) override;

  private:
    juce::TextButton closeButton;
    juce::TextButton confirmButton;

    ActivityManager &activityManager;

    juce::SharedResourcePointer<Config> sharedConfig;

    std::string selectedFolder;

    ProjectsDataFrame rowManager;
    Table projectsTable;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OpenProjectDialog)
};

#endif // DEF_OPEN_PROJECT_DIALOG_HPP