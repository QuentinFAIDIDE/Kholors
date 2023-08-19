#ifndef DEF_INSTANTIATE_GIT_DIALOG_HPP
#define DEF_INSTANTIATE_GIT_DIALOG_HPP

#include "../../Arrangement/ActivityManager.h"
#include <juce_gui_basics/juce_gui_basics.h>

#include <regex>

#define GIT_INIT_REPO_DIALOG_WIDTH 340
#define GIT_INIT_REPO_DIALOG_HEIGHT 86

/**
 * @brief      This class describes a git repository instanciation dialog for current
 *             song opened in arrangement.
 */
class GitInitRepoDialog : public juce::Component, juce::Button::Listener, juce::TextEditor::Listener
{
  public:
    /**
     * @brief      Constructs a new instance.
     */
    GitInitRepoDialog(ActivityManager &am);

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

    /** Called when the user changes the text in some way. */
    void textEditorTextChanged(juce::TextEditor &) override;

    /** Called when the user presses the return key. */
    void textEditorReturnKeyPressed(juce::TextEditor &) override;

  private:
    juce::TextButton closeButton;
    juce::TextButton confirmButton;
    juce::TextEditor nameEntry;

    std::string chosenTrackName;
    std::regex contentValidationRegex;

    ActivityManager &activityManager;

    /////////////////////////////////////////

    bool nameIsValid();

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GitInitRepoDialog)
};

#endif // DEF_INSTANTIATE_GIT_DIALOG_HPP