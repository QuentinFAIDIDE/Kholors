#ifndef DEF_PASSCLICK_TAB_HPP
#define DEF_PASSCLICK_TAB_HPP

#include <juce_gui_basics/juce_gui_basics.h>

#include "../../Config.h"

/**
 * @brief This class describes a TabbedComponent
 *        from juce, but with the added ability
 *        to have an area on the top that is passing
 *        clicks to parents so that we can initiate a
 *        resize.
 *
 */
class ResizableTabsContainer : public juce::TabbedComponent
{
  public:
    ResizableTabsContainer(juce::TabbedButtonBar::Orientation a) : juce::TabbedComponent(a){};

    /**
     * @brief Determines if we intercept the click or not (juce calls it).
     *
     * @param x x coordinate in pixel inside local area (from topleft)
     * @param y y coordinate in pixel inside local area (from topleft)
     * @return true if click is intercepted
     * @return false if click is not intercepted.
     */
    bool hitTest(int, int y) override
    {
        if (y < MAINVIEW_RESIZE_HANDLE_HEIGHT >> 1)
        {
            return false;
        }
        return true;
    };
};

#endif // DEF_PASSCLICK_TAB_HPP