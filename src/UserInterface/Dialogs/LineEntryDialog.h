#ifndef LINE_ENTRY_DIALOG_HPP
#define LINE_ENTRY_DIALOG_HPP

#include "../../Arrangement/ActivityManager.h"
#include "../Section.h"
#include <regex>

// An abstract class describing a dialog with a one line text entry, a cancel and a confirm button that is only
// toggled if it matches some regex.
class LineEntryDialog : public juce::Component, juce::Button::Listener, juce::TextEditor::Listener
{
  public:
    /**
     * @brief      Constructs a new instance.
     */
    LineEntryDialog(ActivityManager &am);

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

    /**
     * @brief      Return the activity manager for this object.
     *
     * @return     The activity manager.
     */
    ActivityManager &getActivityManager();

    /**
     * @brief      Pull the overriden child implementations to set the various
     *             properties of the interface.
     */
    void initializeDialog();

    /**
     * @brief      Gets the text entry validation regular expression.
     *
     * @return     A regular expression that passes if text inside text entry component is okay.
     */
    virtual std::regex getEntryRegex() = 0;

    /**
     * @brief      Gets the text entry description.
     *
     * @return     The text entry description.
     */
    virtual std::string getTextEntryDescription() = 0;

    /**
     * @brief      Gets the dialog width.
     *
     * @return     The dialog width.
     */
    virtual int getDialogWidth() = 0;

    /**
     * @brief      Gets the dialog height.
     *
     * @return     The dialog height.
     */
    virtual int getDialogHeight() = 0;

    /**
     * @brief      Gets the line of text displayed above text entry
     *             that describe what to enter.
     *
     * @return     The dialog instructions.
     */
    virtual std::string getDialogInstructions() = 0;

    /**
     * @brief      Performs dialog task when user click the confirm button.
     *
     * @param[in]  dialogEntry  The dialog entry text.
     */
    virtual void performDialogTask(std::string dialogEntry) = 0;

  private:
    juce::TextButton closeButton;
    juce::TextButton confirmButton;
    juce::TextEditor textEntry;

    std::string textEntryMessage;
    std::regex contentValidationRegex;

    ActivityManager &activityManager;

    /////////////////////////////////////////

    bool nameIsValid();

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LineEntryDialog)
};

#endif // LINE_ENTRY_DIALOG_HPP