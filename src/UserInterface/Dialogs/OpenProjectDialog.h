#ifndef DEF_OPEN_PROJECT_DIALOG_HPP
#define DEF_OPEN_PROJECT_DIALOG_HPP

#include "../../Arrangement/ActivityManager.h"
#include "../Section.h"

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

    /**
     * @brief      Return the activity manager for this object.
     *
     * @return     The activity manager.
     */
    ActivityManager &getActivityManager();

  private:
    juce::TextButton closeButton;
    juce::TextButton confirmButton;
    Table projectsTable;

    juce::SharedResourcePointer<Config> sharedConfig;
    ActivityManager &activityManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OpenProjectDialog)
};

#endif // DEF_OPEN_PROJECT_DIALOG_HPP