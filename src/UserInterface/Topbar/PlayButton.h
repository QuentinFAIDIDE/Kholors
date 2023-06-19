#ifndef DEF_PLAY_BUTTON_HPP
#define DEF_PLAY_BUTTON_HPP

#include "../../Arrangement/ActivityManager.h"
#include "../IconsLoader.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

/**
 * @brief      A play button component that changes icon
 *             based on playback state.
 */
class PlayButton : public juce::Component, public TaskListener
{
  public:
    /**
     * @brief      Constructs a new instance.
     */
    PlayButton(ActivityManager &am);

    /**
     * @brief      Receives playback state updates.
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

    // is the track currently playing ?
    bool isPlaying;
};

#endif // DEF_PLAY_BUTTON_HPP