#ifndef DEF_MENU_BAR_HPP
#define DEF_MENU_BAR_HPP

#include <juce_gui_basics/juce_gui_basics.h>

#include "../Arrangement/ActivityManager.h"

#define MENU_BAR_HEIGHT 25

/**
 * @brief      This class describes the topbar menu bar.
 */
class MenuBar : public juce::Component
{
  public:
    /**
     * @brief      Constructs a new instance.
     *
     * @param      am    A reference to the activity manager we can send tasks to.
     */
    MenuBar(ActivityManager &am);

    /**
     * @brief      Called when this components needs repainting.
     *
     * @param      g     The graphics object allowing to drawn on screen.
     */
    void paint(juce::Graphics &g) override;

    /**
     * @brief      Called when the mouse is clicked over that component.
     *
     * @param[in]  me    Mouse event object.
     */
    void mouseDown(const juce::MouseEvent &me) override;

    /**
     * @brief      Called when the mouse is moved over the component.
     *
     * @param[in]  me    Mouse event object.
     */
    void mouseMove(const juce::MouseEvent &me) override;

    /**
     * @brief      Called when the mouse exits the component.
     */
    void mouseExit(const juce::MouseEvent &me) override;

  private:
    // reference to the activity manager to send it tasks on menu clicks
    ActivityManager &activityManager;

    // id of the menu item that is open
    int openedMenuId;

    // id set to the id of the id the mouse is over
    int mouseOverId;

    // cached menu rectangles in local coordinates for click use
    std::map<int, juce::Rectangle<int>> menuItemsRectangles;

    ///////////

    /**
     * @brief      Draws a menu item.
     *
     * @param[in]  id      The identifier of the menu. Starts at 1 from left to right.
     * @param[in]  text    The text of the menu.
     * @param      bounds  The bounds from which we remove space from the left to get area to draw at.
     * @param      g       graphics context to draw
     */
    void drawMenuItem(int id, std::string text, juce::Rectangle<int> &bounds, juce::Graphics &g);

    void openFileMenu();
    void openHelpMenu();
    void openVersionningMenu();
    void openEditMenu();
};

#endif // DEF_MENU_BAR_HPP