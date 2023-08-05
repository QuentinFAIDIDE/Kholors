#ifndef DEF_MENU_BAR_HPP
#define DEF_MENU_BAR_HPP

#include <juce_gui_basics/juce_gui_basics.h>

#include "../../Arrangement/ActivityManager.h"

/**
 * @brief      This class describes a menu bar component. Note that
 *             we inherit both the component and model to be more
 *             concise.
 */
class MenuBar : public juce::MenuBarComponent, juce::MenuBarModel
{
  public:
    /**
     * @brief      Constructs a new instance.
     *
     * @param      am    A reference to the activity manager we can send tasks to.
     */
    MenuBar(ActivityManager &am);

    /**
     * @brief      Gets the menu bar names.
     *
     * @return     The menu bar names.
     */
    juce::StringArray getMenuBarNames() override;

    /**
     * @brief      Gets the menu for index.
     *
     * @param[in]  topLevelMenuIndex  The top level menu index
     * @param[in]  menuName           The menu name
     *
     * @return     The menu for index.
     */
    juce::PopupMenu getMenuForIndex(int topLevelMenuIndex, const juce::String &menuName) override;

    /**
     * @brief      This is called when a menu item has been clicked on.
     *
     * @param[in]  menuItemID         The menu item id
     * @param[in]  topLevelMenuIndex  The top level menu index
     */
    void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;

  private:
};

#endif // DEF_MENU_BAR_HPP