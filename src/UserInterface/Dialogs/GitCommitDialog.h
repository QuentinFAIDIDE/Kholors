#ifndef DEF_GIT_COMMIT_DIALOG_HPP
#define DEF_GIT_COMMIT_DIALOG_HPP

#include "LineEntryDialog.h"

#include <regex>

#define GIT_COMMIT_DIALOG_WIDTH 340
#define GIT_COMMIT_DIALOG_HEIGHT 86

/**
 * @brief      This class describes a git commit dialog for current
 *             song opened in arrangement.
 */
class GitCommitDialog : public LineEntryDialog
{
  public:
    GitCommitDialog(ActivityManager &am);

    /**
     * @brief      Gets the text entry validation regular expression.
     *
     * @return     A regular expression that passes if text inside text entry component is okay.
     */
    std::regex getEntryRegex();

    /**
     * @brief      Gets the text entry description.
     *
     * @return     The text entry description.
     */
    std::string getTextEntryDescription();

    /**
     * @brief      Gets the dialog width.
     *
     * @return     The dialog width.
     */
    int getDialogWidth();

    /**
     * @brief      Gets the dialog height.
     *
     * @return     The dialog height.
     */
    int getDialogHeight();

    /**
     * @brief      Gets the line of text displayed above text entry
     *             that describe what to enter.
     *
     * @return     The dialog instructions.
     */
    std::string getDialogInstructions();

    /**
     * @brief      Performs dialog task when user click the confirm button.
     *
     * @param[in]  dialogEntry  The dialog entry text.
     */
    void performDialogTask(std::string dialogEntry);

  private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GitCommitDialog)
};

#endif // DEF_GIT_COMMIT_DIALOG_HPP