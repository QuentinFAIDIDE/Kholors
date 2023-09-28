#include "NotificationOverlay.h"
#include "../Config.h"
#include <bits/chrono.h>
#include <chrono>
#include <memory>

// somehow we end up requiring this
#define NOTIF_BODY_TOP_PADDING 12

NotificationOverlay::NotificationOverlay()
{
}

void NotificationOverlay::paint(juce::Graphics &g)
{
    // isolate bottom band where we will display notifications
    auto notifBottomBand = getLocalBounds().removeFromBottom(NOTIF_HEIGHT + NOTIF_OUTTER_MARGINS);
    notifBottomBand.reduce(NOTIF_OUTTER_MARGINS >> 1, NOTIF_OUTTER_MARGINS >> 1);

    // clear the areas of the notif that we will refill just now
    shownNotifAreas.clear();

    // index of the notification shown on them
    int notifId = 0;

    // iterate over notification and draw them
    for (auto it = displayedNotifs.begin(); it != displayedNotifs.end(); it++)
    {
        // increment notif index
        notifId++;

        // compute the notification area
        auto notifArea = notifBottomBand.removeFromRight(NOTIF_WIDTH);
        notifBottomBand.removeFromRight(NOTIF_PADDING);

        // paint the notification in its area
        auto notifBox = paintNotification(g, notifId, *it, notifArea);

        // save a reference to its position and index for handling clicks
        shownNotifAreas.push_back(notifBox);
    }
}

NotificationBoxArea NotificationOverlay::paintNotification(juce::Graphics &g, int notifIndex, Notification &notif,
                                                           juce::Rectangle<int> &area)
{
    g.setColour(COLOR_BACKGROUND_LIGHTER);
    g.fillRect(area);

    g.setColour(COLOR_TEXT);
    g.drawRect(area, 1);

    // pick the color depending on the type of notification
    juce::Colour headerColor;
    std::string headerTitle;
    switch (notif.type)
    {
    case INFO_NOTIF_TYPE:
        headerColor = COLOR_NOTIF_INFO;
        headerTitle = "INFO";
        break;

    case WARNING_NOTIF_TYPE:
        headerColor = COLOR_NOTIF_WARN;
        headerTitle = "WARNING";
        break;

    case ERROR_NOTIF_TYPE:
        headerColor = COLOR_NOTIF_ERROR;
        headerTitle = "ERROR";
        break;
    }

    // notif header area with the title and cross
    auto areaCopy = area;
    auto headerArea = areaCopy.removeFromTop(TABS_HEIGHT);

    // draw the line on header bottom
    g.setColour(headerColor);
    auto headerBottomLine = headerArea;
    headerBottomLine = headerBottomLine.removeFromBottom(TAB_HIGHLIGHT_LINE_WIDTH);
    g.fillRect(headerBottomLine);

    // add margins to side of headers
    headerArea.reduce(NOTIF_INNER_MARGINS >> 1, 0);

    // draw the number of the notification in the top left corner
    auto numArea = headerArea.removeFromLeft(headerArea.getHeight());
    g.setColour(COLOR_TEXT);
    g.setFont(juce::Font(DEFAULT_FONT_SIZE));
    g.drawText(std::to_string(notifIndex), numArea, juce::Justification::centred, false);

    // draw the cross to the right
    auto crossArea = headerArea.removeFromRight(headerArea.getHeight());
    crossArea.reduce(12, 12);
    crossArea.setX(crossArea.getX() + 6);
    g.drawLine(juce::Line(crossArea.getTopLeft().toFloat(), crossArea.getBottomRight().toFloat()), 3);
    g.drawLine(juce::Line(crossArea.getTopRight().toFloat(), crossArea.getBottomLeft().toFloat()), 3);

    // draw the notification title
    g.setColour(headerColor);
    headerArea.reduce(NOTIF_INNER_MARGINS >> 1, 0);
    g.drawText(headerTitle, headerArea, juce::Justification::centred, true);

    // draw the main body of text
    g.setColour(COLOR_TEXT);
    area.reduce(NOTIF_INNER_MARGINS >> 1, NOTIF_INNER_MARGINS >> 1);
    g.drawMultiLineText(notif.message, areaCopy.getTopLeft().getX(),
                        areaCopy.getTopLeft().getY() + NOTIF_BODY_TOP_PADDING, areaCopy.getWidth());
}

void NotificationOverlay::update()
{
    // prevent race conditions, and allow to call component repaint
    const juce::MessageManagerLock mml;

    // get timestamp of now
    auto now = std::chrono::system_clock::now().time_since_epoch();
    unsigned int nowTimestamp = std::chrono::duration_cast<std::chrono::seconds>(now).count();

    // iterate over notifications and delete expired ones
    for (size_t i = 0; i < displayedNotifs.size(); i++)
    {
        if (displayedNotifs[i].expirationTimeUnix < nowTimestamp)
        {
            displayedNotifs.erase(displayedNotifs.begin() + (int)i);

            // if no more task to watch, stop the timer
            if (displayedNotifs.size() == 0)
            {
                stopTimer();
            }

            repaint();
            return;
        }
    }
}

bool NotificationOverlay::taskHandler(std::shared_ptr<Task> task)
{
    auto notifTask = std::dynamic_pointer_cast<NotificationTask>(task);
    if (notifTask != nullptr && !notifTask->isCompleted() && !notifTask->hasFailed())
    {
        displayedNotifs.push_back(notifTask->content);
        repaint();
        notifTask->setCompleted(true);

        // ensure timer if delete outdated tasks is startede
        if (!isTimerRunning())
        {
            startTimer(NOTIF_UPDATE_INTERVAL_MS);
        }

        return true;
    }

    return false;
}

bool NotificationOverlay::hitTest(int x, int y)
{
    for (size_t i = 0; i < shownNotifAreas.size(); i++)
    {
        if (shownNotifAreas[i].first.contains(x, y))
        {
            return true;
        }
    }

    return false;
}

void NotificationOverlay::timerCallback()
{
    update();
}