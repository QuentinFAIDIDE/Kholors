#ifndef DEF_NOTIF_OVERLAY_HPP
#define DEF_NOTIF_OVERLAY_HPP

#include "../Arrangement/ActivityManager.h"
#include "../Notification.h"
#include <juce_gui_extra/juce_gui_extra.h>

#define NOTIF_UPDATE_INTERVAL_MS 1000

/**
 * @brief Struct that contains notification box
 *        id and location on screen (for box and exit button).
 */
struct NotificationBoxArea
{
    NotificationBoxArea(int i, juce::Rectangle<int> area, juce::Rectangle<int> eb)
    {
        id = i;
        box = area;
        exitButton = eb;
    }

    int id;                   /**< Identifier of the notification */
    juce::Rectangle<int> box; /**< Rectangle of the notification box (in local Notif Overlay widget coordinates) */
    juce::Rectangle<int> exitButton; /**< Rectangle with exit button (in local Notif Overlay widget coordinates) */
};

/**
 * @brief A component that is over the app and will display notifications
 *        while leaving mouse events that are not on notifications go through.
 *
 */
class NotificationOverlay : public juce::Component, public TaskListener, public juce::Timer
{
  public:
    /**
     * @brief Construct a new Notif Modals Overlay object
     *
     */
    NotificationOverlay();

    /**
     * @brief Paint the notifications if there are.
     */
    void paint(juce::Graphics &) override;

    /**
     * @brief A callback for the timer we inherit from (used to call udpate and check for notif deletion)
     *
     */
    void timerCallback() override;

    /**
     * @brief Called by time when animated app component has to update every NOTIF_UPDATE_INTERVAL_MS ms.
     */
    void update();

    /**
     * @brief Used to determine if we intercept the mouse event or pass it to parents.
     *
     * @param x local x coordinates of the mouse event
     * @param y local y coordinates of the mouse event
     * @return true
     * @return false
     */
    bool hitTest(int x, int y) override;

    /**
     * @brief Task receiver.
     *
     * @param task the task that is broadcasted through activityManager
     * @return true If the task should be intercepted and not broadcasted further.
     * @return false If the task should be let through.
     * Note that generally you intercept a task when you need to repost it and be sure some
     * other task listener gets it with your modified fields.
     */
    bool taskHandler(std::shared_ptr<Task> task) override;

  private:
    std::vector<Notification> displayedNotifs; /**< Notifications to display on-screen. Will be removed
                                                                  by update when timing out */

    std::vector<NotificationBoxArea>
        shownNotifAreas; /**< Area where notif are displayed, as well as their unique identifier. */

    /**
     * @brief Paint a specific notification box (there are many that can be shown).
     *
     * @param g Juce graphical drawing context.
     * @param notifIndex Index of the notification from 1 to how many there are
     * @param notif Notification object holding information
     * @param area Area to draw the notification in
     */
    NotificationBoxArea paintNotification(juce::Graphics &g, int notifIndex, Notification &notif,
                                          juce::Rectangle<int> &area);
};

#endif // DEF_NOTIF_OVERLAY_HPP