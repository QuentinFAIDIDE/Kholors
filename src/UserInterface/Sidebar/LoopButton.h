#ifndef DEF_LOOP_BUTTON_HPP
#define DEF_LOOP_BUTTON_HPP

#include "../../Arrangement/ActivityManager.h"
#include "../IconsLoader.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

/**
 * @brief      A loop button component that changes icon
 *             based on loop state.
 */
class LoopButton : public juce::Component, public TaskListener
{
  public:
    /**
     * @brief      Constructs a new instance.
     */
    LoopButton(ActivityManager &am);

    /**
     * @brief      Receives loop state updates.
     */
    bool taskHandler(std::shared_ptr<Task>) override;

    /**
     * @brief      Paints the component
     *
     */
    void paint(juce::Graphics &) override;

    /**
     * @brief      When mouse is clicked.
     */
    void mouseDown(const juce::MouseEvent &) override;

  private:
    /**
     * a reference to the activity manager that allows
     * to submit tasks and register as a task listener.
     */
    ActivityManager &activityManager;

    /**
     * Shared svg icons.
     */
    juce::SharedResourcePointer<IconsLoader> sharedIcons;

    // is loop mode toggled on ?
    bool isLooping;
};

#endif // DEF_LOOP_BUTTON_HPP