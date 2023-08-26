#ifndef DEF_OPEN_PROJECT_DIALOG_HPP
#define DEF_OPEN_PROJECT_DIALOG_HPP

#include "../../Arrangement/ActivityManager.h"
#include "../Section.h"
#include "../Widgets/Table.h"

class ProjectsDataFrame : public TableDataFrame
{
  public:
    ProjectsDataFrame(std::string projectFolderPath);
    int getMaxRowNumber();
    std::vector<TableCell> &getRow(int n);
    std::vector<std::pair<TableType, TableColumnAlignment>> &getFormat();
    std::optional<std::pair<int, bool>> &getOrdering();
    bool trySettingOrdering(std::pair<int, bool> ordering);
    std::vector<std::string> &getColumnNames();

    bool sortByCreationDate(int a, int b) const;
    bool sortByLastEditDate(int a, int b) const;

  private:
    std::vector<std::string> projectsFoldersNames; /**< projects folder names (in loading order) */
    std::vector<std::time_t>
        projectsFoldersLastModifiedTimeSec; /**< projects folder last edit timestamps (in loading order) */
    std::vector<std::time_t>
        projectsFoldersCreatedTimeSec;               /**< projects folder creation timestamps (in loading order) */
    std::map<int, std::vector<TableCell>> rowsCache; /**< rows that are cached */
    std::optional<std::pair<int, bool>> ordering;    /**< column index of ordering and bool to know if ascending */
    std::vector<std::pair<TableType, TableColumnAlignment>> format; /**< column types and alignments */
    std::vector<std::string> colNames;                              /**< header col names */
    std::vector<int> orderedIds;                                    /**<
                                                                     vector of indexes of projectsFoldersNames rows that are sorted in specified ordering.
                                                                    */

    /**
     * @brief      Formats timestamp into appropriate time
     *
     * @param      t    Unix timestamp in seconds.
     *
     * @return     stringified time in configured format.
     */
    std::string formatDatetime(std::time_t &t);
};

class OpenProjectDialog : public juce::Component, juce::Button::Listener
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

  private:
    juce::TextButton closeButton;
    juce::TextButton confirmButton;

    Table projectsTable;

    juce::SharedResourcePointer<Config> sharedConfig;
    ActivityManager &activityManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OpenProjectDialog)
};

#endif // DEF_OPEN_PROJECT_DIALOG_HPP