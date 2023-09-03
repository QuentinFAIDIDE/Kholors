#ifndef DEF_CONFIRM_DIALOG_HPP
#define DEF_CONFIRM_DIALOG_HPP

#include "../../Arrangement/ActivityManager.h"
#include <juce_gui_extra/juce_gui_extra.h>

#define CONFIRM_DIALOG_CONTENT_PADDING 6
#define CONFIRM_DIALOG_TEXT_SIZE 16
#define CONFIRM_TEXT_TOP_PADDING 32

/**
 * @brief      An abstract class that implements a dialog with a simple
 *             bloc of text as well as a confirm or cancel button.
 *             It performs what is defined in the performDialogTask callback.
 */
class ConfirmDialog : public juce::Component, juce::Button::Listener
{
  public:
    /**
     * @brief      Constructs a new instance with a reference to the activityManager
     *             that is supposed to receive and perform new tasks.
     */
    ConfirmDialog(ActivityManager &am);

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
     * @brief      Return the text that is displayed in the dialog window.
     *
     * @return     The dialog text.
     */
    virtual std::string getDialogText() = 0;

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
     * @brief      Performs dialog task when user click the confirm button.
     *
     */
    virtual void performDialogTask() = 0;

  private:
    juce::TextButton closeButton;
    juce::TextButton confirmButton;
    std::string dialogMessage;

    ActivityManager &activityManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConfirmDialog)
};

#endif // DEF_CONFIRM_DIALOG_HPP