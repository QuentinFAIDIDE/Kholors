#include "MenuBar.h"

MenuBar::MenuBar(ActivityManager &am)
{
    // we tell the menubarcomponent we inherit from to
    // use the menubarmodel we inherit from.
    setModel(this);
}

juce::StringArray MenuBar::getMenuBarNames()
{
    return juce::StringArray({"File", "Help"});
}

juce::PopupMenu MenuBar::getMenuForIndex(int topLevelMenuIndex, const juce::String &menuName)
{
    juce::PopupMenu menu = juce::PopupMenu();
    menu.addItem(1, "Menu Item");
    menu.addItem(2, "Other Item");
    return menu;
}

void MenuBar::menuItemSelected(int menuItemID, int topLevelMenuIndex)
{
    // TODO
}
