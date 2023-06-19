#ifndef DEF_STOP_BUTTON_HPP
#define DEF_STOP_BUTTON_HPP

#include "PlayButton.h"

/**
 * @brief      A stop button component that changes icon
 *             based on playback state.
 */
class StopButton : public PlayButton
{
  public:
    StopButton(ActivityManager &);
    /**
     * @brief      Paints the component
     *
     */
    void paint(juce::Graphics &) override;

    /**
     * @brief      When mouse is clicked.
     */
    void mouseDown(const juce::MouseEvent &) override;
};

#endif // DEF_STOP_BUTTON_HPP