#include "MenuBar.h"
#include "../Config.h"
#include <memory>

#include "./Dialogs/ConfirmResetDialog.h"
#include "./Dialogs/GitCommitDialog.h"
#include "./Dialogs/GitInitRepoDialog.h"
#include "./Dialogs/OpenProjectDialog.h"

#define FILE_MENU_ID 1
#define FILE_MENU_TEXT "File"
#define FILE_MENU_ITEM_ID_QUIT 1000
#define FILE_MENU_ITEM_ID_NEW 1001
#define FILE_MENU_ITEM_ID_OPEN 1002
#define FILE_MENU_ITEM_ID_OPEN_RECENT 1003

#define EDIT_MENU_ID 2
#define EDIT_MENU_TEXT "Edit"
#define EDIT_MENU_ITEM_ID_UNDO 2000
#define EDIT_MENU_ITEM_ID_REDO 2001

#define VERSIONNING_MENU_ID 3
#define VERSIONNING_MENU_TEXT "Project"
#define VERSIONNING_MENU_ITEM_ID_INIT 3001
#define VERSIONNING_MENU_ITEM_ID_COMMIT 3002
#define VERSIONNING_MENU_ITEM_ID_RESET 3003
#define VERSIONNING_MENU_ITEM_ID_REVERT 3004
#define VERSIONNING_MENU_ITEM_ID_CHANGE_BRANCH 3005
#define VERSIONNING_MENU_ITEM_ID_CREATE_BRANCH 3006

#define HELP_MENU_ID 4
#define HELP_MENU_TEXT "Help"
#define HELP_MENU_ITEM_ID_MANUAL 4000
#define HELP_MENU_ITEM_ID_SHORTCUTS 4001
#define HELP_MENU_ITEM_ID_ABOUT 4002

#define NO_MENU_ITEMS 4

#define MENU_LEFT_MARGIN 16
#define MENU_RIGHT_MARGIN 8
#define MENU_SEPARATOR_WIDTH 1
#define OPENED_MENU_BOTTOM_LINE_WIDTH 2
#define POPUP_MENU_Y_SHIFT 2

MenuBar::MenuBar(ActivityManager &am) : activityManager(am)
{
    openedMenuId = 0;
    mouseOverId = 0;
}

void MenuBar::paint(juce::Graphics &g)
{
    auto bounds = getLocalBounds();
    g.setColour(COLOR_BACKGROUND);
    g.fillAll();
    drawMenuItem(1, FILE_MENU_TEXT, bounds, g);
    drawMenuItem(2, EDIT_MENU_TEXT, bounds, g);
    drawMenuItem(3, VERSIONNING_MENU_TEXT, bounds, g);
    drawMenuItem(4, HELP_MENU_TEXT, bounds, g);
}

void MenuBar::mouseDown(const juce::MouseEvent &me)
{
    for (int i = 1; i <= NO_MENU_ITEMS; i++)
    {
        if (menuItemsRectangles.find(i) != menuItemsRectangles.end())
        {
            if (menuItemsRectangles[i].contains(me.position.toInt()))
            {
                // we don't open the same menu twice to prevent
                // issues on popup menu closing when it needs to reset
                // the currently opened id.
                // The issue is due to our use of a unique opened menu index and the fact
                // that the closing of the first popup don't necessarely happen
                // before the new one opens in JUCE, making it harder to reset opened id to 0.
                // We can actually do a more advanced setup and allow
                // that menu reopen, but we choose to keep the code simple at this
                // tiny feature cost.
                // hence the if below
                if (i != openedMenuId)
                {
                    if (i == FILE_MENU_ID)
                    {
                        openFileMenu();
                        return;
                    }

                    if (i == EDIT_MENU_ID)
                    {
                        openEditMenu();
                        return;
                    }

                    if (i == VERSIONNING_MENU_ID)
                    {
                        openVersionningMenu();
                        return;
                    }

                    if (i == HELP_MENU_ID)
                    {
                        openHelpMenu();
                        return;
                    }
                }
            }
        }
    }
}

void MenuBar::mouseMove(const juce::MouseEvent &me)
{
    int previousMouseOverId = mouseOverId;

    for (int i = 1; i <= NO_MENU_ITEMS; i++)
    {
        if (menuItemsRectangles.find(i) != menuItemsRectangles.end())
        {
            if (menuItemsRectangles[i].contains(me.position.toInt()))
            {
                mouseOverId = i;
                if (mouseOverId != previousMouseOverId)
                {
                    repaint();
                }
                return;
            }
        }
    }

    mouseOverId = 0;
    if (mouseOverId != previousMouseOverId)
    {
        repaint();
    }
}

void MenuBar::mouseExit(const juce::MouseEvent &)
{
    mouseOverId = 0;
}

void MenuBar::drawMenuItem(int id, std::string text, juce::Rectangle<int> &bounds, juce::Graphics &g)
{
    auto fileMenuWidth = juce::Font(DEFAULT_FONT_SIZE).getStringWidth(text);
    auto fileMenuArea = bounds.removeFromLeft(MENU_LEFT_MARGIN + fileMenuWidth + MENU_RIGHT_MARGIN);

    g.setColour(COLOR_TEXT_DARKER);

    if (id == openedMenuId)
    {
        auto bottomLine =
            juce::Line<float>(fileMenuArea.getBottomLeft().toFloat(), fileMenuArea.getBottomRight().toFloat());
        g.drawLine(bottomLine, OPENED_MENU_BOTTOM_LINE_WIDTH);
    }

    if (id == mouseOverId)
    {
        g.setColour(COLOR_TEXT);
    }

    menuItemsRectangles[id] = fileMenuArea;

    g.setFont(DEFAULT_FONT_SIZE);
    fileMenuArea.removeFromLeft(MENU_LEFT_MARGIN);
    fileMenuArea.removeFromRight(MENU_RIGHT_MARGIN);
    g.setFont(juce::Font(DEFAULT_FONT_SIZE));
    g.drawText(text, fileMenuArea, juce::Justification::centredLeft, false);
}

void MenuBar::openFileMenu()
{
    openedMenuId = FILE_MENU_ID;

    juce::PopupMenu recentFilesMenu;
    recentFilesMenu.addItem(10000, "To be implemented...", false, false);

    juce::PopupMenu menu;
    menu.addItem(FILE_MENU_ITEM_ID_NEW, "New");
    menu.addSeparator();
    menu.addItem(FILE_MENU_ITEM_ID_OPEN, "Open");
    menu.addSubMenu("Open Recent", recentFilesMenu, true);
    menu.addSeparator();
    menu.addItem(FILE_MENU_ITEM_ID_QUIT, "Quit");
    menu.setLookAndFeel(&getLookAndFeel());

    juce::Rectangle<int> targetArea;
    targetArea.setX(getScreenX() + menuItemsRectangles[FILE_MENU_ID].getX());
    targetArea.setY(getScreenY() + menuItemsRectangles[FILE_MENU_ID].getY() +
                    menuItemsRectangles[FILE_MENU_ID].getHeight() + POPUP_MENU_Y_SHIFT);

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea(targetArea), [this](int id) {
        // only reset if another menu didn't open meanwhile
        if (openedMenuId == FILE_MENU_ID)
        {
            openedMenuId = 0;
        }

        if (id == FILE_MENU_ITEM_ID_QUIT)
        {
            auto quitTask = std::make_shared<QuittingTask>();
            activityManager.broadcastTask(quitTask);
        }

        if (id == FILE_MENU_ITEM_ID_NEW)
        {
            auto resetTask = std::make_shared<ResetTask>();
            activityManager.broadcastTask(resetTask);
        }

        if (id == FILE_MENU_ITEM_ID_OPEN)
        {
            juce::DialogWindow::LaunchOptions launchOptions;
            launchOptions.dialogTitle = "Open existing project";
            launchOptions.content.set(new OpenProjectDialog(activityManager), true);
            launchOptions.escapeKeyTriggersCloseButton = true;
            launchOptions.useNativeTitleBar = true;
            launchOptions.resizable = false;
            launchOptions.useBottomRightCornerResizer = false;
            launchOptions.launchAsync();
        }
    });
}

void MenuBar::openEditMenu()
{
    openedMenuId = EDIT_MENU_ID;

    juce::PopupMenu menu;
    menu.addItem(EDIT_MENU_ITEM_ID_UNDO, "Undo");
    menu.addItem(EDIT_MENU_ITEM_ID_REDO, "Redo");
    menu.setLookAndFeel(&getLookAndFeel());

    juce::Rectangle<int> targetArea;
    targetArea.setX(getScreenX() + menuItemsRectangles[EDIT_MENU_ID].getX());
    targetArea.setY(getScreenY() + menuItemsRectangles[EDIT_MENU_ID].getY() +
                    menuItemsRectangles[EDIT_MENU_ID].getHeight() + POPUP_MENU_Y_SHIFT);

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea(targetArea), [this](int id) {
        // only reset if another menu didn't open meanwhile
        if (openedMenuId == EDIT_MENU_ID)
        {
            openedMenuId = 0;
        }
    });
}

void MenuBar::openVersionningMenu()
{
    openedMenuId = VERSIONNING_MENU_ID;

    bool isRepoAlreadyInitialized = activityManager.getAppState().getRepoDirectory().has_value();

    juce::PopupMenu menu;
    menu.addItem(VERSIONNING_MENU_ITEM_ID_INIT, "Initialize", !isRepoAlreadyInitialized, false);
    menu.addSeparator();
    menu.addItem(VERSIONNING_MENU_ITEM_ID_COMMIT, "Save and commit", isRepoAlreadyInitialized, false);
    menu.addItem(VERSIONNING_MENU_ITEM_ID_RESET, "Reset changes", isRepoAlreadyInitialized, false);
    menu.addSeparator();
    menu.addItem(VERSIONNING_MENU_ITEM_ID_REVERT, "Revert to a past commit", isRepoAlreadyInitialized, false);
    menu.addItem(VERSIONNING_MENU_ITEM_ID_CREATE_BRANCH, "New branch", isRepoAlreadyInitialized, false);
    menu.addItem(VERSIONNING_MENU_ITEM_ID_CHANGE_BRANCH, "Switch branch", isRepoAlreadyInitialized, false);

    menu.setLookAndFeel(&getLookAndFeel());

    juce::Rectangle<int> targetArea;
    targetArea.setX(getScreenX() + menuItemsRectangles[VERSIONNING_MENU_ID].getX());
    targetArea.setY(getScreenY() + menuItemsRectangles[VERSIONNING_MENU_ID].getY() +
                    menuItemsRectangles[VERSIONNING_MENU_ID].getHeight() + POPUP_MENU_Y_SHIFT);

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea(targetArea), [this](int id) {
        // only reset if another menu didn't open meanwhile
        if (openedMenuId == VERSIONNING_MENU_ID)
        {
            openedMenuId = 0;
        }

        if (id == VERSIONNING_MENU_ITEM_ID_INIT)
        {
            juce::DialogWindow::LaunchOptions launchOptions;
            launchOptions.dialogTitle = "Instantiate a new project";
            launchOptions.content.set(new GitInitRepoDialog(activityManager), true);
            launchOptions.escapeKeyTriggersCloseButton = true;
            launchOptions.useNativeTitleBar = true;
            launchOptions.resizable = false;
            launchOptions.useBottomRightCornerResizer = false;
            launchOptions.launchAsync();
        }

        if (id == VERSIONNING_MENU_ITEM_ID_COMMIT)
        {
            juce::DialogWindow::LaunchOptions launchOptions;
            launchOptions.dialogTitle = "Save and commit your changes";
            launchOptions.content.set(new GitCommitDialog(activityManager), true);
            launchOptions.escapeKeyTriggersCloseButton = true;
            launchOptions.useNativeTitleBar = true;
            launchOptions.resizable = false;
            launchOptions.useBottomRightCornerResizer = false;
            launchOptions.launchAsync();
        }

        if (id == VERSIONNING_MENU_ITEM_ID_RESET)
        {
            juce::DialogWindow::LaunchOptions launchOptions;
            launchOptions.dialogTitle = "Reset changes";
            launchOptions.content.set(new ConfirmResetDialog(activityManager), true);
            launchOptions.escapeKeyTriggersCloseButton = true;
            launchOptions.useNativeTitleBar = true;
            launchOptions.resizable = false;
            launchOptions.useBottomRightCornerResizer = false;
            launchOptions.launchAsync();
        }
    });
}

void MenuBar::openHelpMenu()
{
    openedMenuId = HELP_MENU_ID;

    juce::PopupMenu menu;
    menu.addItem(HELP_MENU_ITEM_ID_MANUAL, "Manual");
    menu.addItem(HELP_MENU_ITEM_ID_SHORTCUTS, "Show shortcuts");
    menu.addSeparator();
    menu.addItem(HELP_MENU_ITEM_ID_ABOUT, "About this version");
    menu.setLookAndFeel(&getLookAndFeel());

    juce::Rectangle<int> targetArea;
    targetArea.setX(getScreenX() + menuItemsRectangles[HELP_MENU_ID].getX());
    targetArea.setY(getScreenY() + menuItemsRectangles[HELP_MENU_ID].getY() +
                    menuItemsRectangles[HELP_MENU_ID].getHeight() + POPUP_MENU_Y_SHIFT);

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea(targetArea), [this](int id) {
        // only reset if another menu didn't open meanwhile
        if (openedMenuId == HELP_MENU_ID)
        {
            openedMenuId = 0;
        }
    });
}