#include "SidebarArea.h"

#define SECTION_COMPONENTS_PADDING 6

SidebarArea::SidebarArea(ActivityManager &am)
    : playButton(am), stopButton(am), loopButton(am), colorPicker(am), trackProperties(am), sampleProperties(am),
      selectionGainVu("SELECTION", VUMETER_ID_SELECTED), masterGainVu("MASTER", VUMETER_ID_MASTER)
{

    isHidden = true;
    isAnimationRunning = false;
    notifBaseX = 0;
    notifBaseY = 0;
    popupY = NOTIF_OUTTER_MARGINS;
    popupX = NOTIF_WIDTH + (2 * NOTIF_OUTTER_MARGINS);
    setFramesPerSecond(NOTIF_ANIM_FPS);
    animationNormalisingFactor = float(NOTIF_WIDTH + (2.0 * NOTIF_OUTTER_MARGINS));

    addAndMakeVisible(playButton);
    addAndMakeVisible(stopButton);
    addAndMakeVisible(loopButton);
    addAndMakeVisible(colorPicker);
    addAndMakeVisible(trackProperties);
    addAndMakeVisible(sampleProperties);
    addAndMakeVisible(selectionGainVu);
    addAndMakeVisible(masterGainVu);
}

SidebarArea::~SidebarArea()
{
}

void SidebarArea::setDataSource(std::shared_ptr<MixbusDataSource> ds)
{
    selectionGainVu.setDataSource(ds);
    masterGainVu.setDataSource(ds);
    trackProperties.setDataSource(ds);
}

bool SidebarArea::taskHandler(std::shared_ptr<Task> task)
{
    std::shared_ptr<NotificationTask> notif = std::dynamic_pointer_cast<NotificationTask>(task);

    if (notif != nullptr && !notif->isCompleted() && !notif->hasFailed())
    {
        notifyMessage(notif->getMessage());
        return true;
    }

    return false;
}

void SidebarArea::paint(juce::Graphics &g)
{
    g.setColour(COLOR_BACKGROUND_LIGHTER);
    g.fillAll();
    g.setColour(COLOR_TEXT);
    g.drawText("MyProject", projectTitleArea, juce::Justification::centredLeft, true);
}

void SidebarArea::paintOverChildren(juce::Graphics &g)
{
    // make sure the notification are filetered to remove old ones
    trimNotifications();

    // if we can't get a read lock, skip any action
    if (!queueRwMutex.tryEnterRead())
    {
        return;
    }

    // TODO: do nothing whenever nothing changed
    // and nothing requires anything to start or end.

    // get the window width
    bounds = g.getClipBounds();

    // base location of the notification area
    notifBaseX = bounds.getWidth() - 2 * NOTIF_OUTTER_MARGINS - NOTIF_WIDTH;

    // get time of now to reuse it in the animation drawings parts
    now = (int)juce::Time::getMillisecondCounter();

    // if we are currently performing an animation
    if (isAnimationRunning)
    {
        timeSinceAnimStart = now - animStartTime;

        // the distance shift from the beginning of the anim
        int distanceDiff = int(
            // make it over 1 to use the ease in function
            easeIn((NOTIF_ANIMATION_SPEED_PIXEL_MS * float(timeSinceAnimStart)) / animationNormalisingFactor)
            // scale it back up after applying ease in function
            * animationNormalisingFactor);

        // if it's going out go left
        if (isHidden == true)
        {
            popupX = NOTIF_OUTTER_MARGINS + distanceDiff;
            // if it reached destination, stop it
            if (popupX >= destinationX)
            {
                popupX = destinationX;
                isAnimationRunning = false;
            }

            // if it's going in go right
        }
        else
        {
            // new position is the screen limit minus the notif movement toward
            // center. we make the fade in faster than fade out for aesthetic reasons
            popupX = NOTIF_WIDTH + (2 * NOTIF_OUTTER_MARGINS) - int(float(distanceDiff) * 4.0);
            // if it reached destination, stop it
            if (popupX <= destinationX)
            {
                popupX = destinationX;
                isAnimationRunning = false;
            }
        }

        // if we are not performing an animation
    }
    else
    {
        // if we need to fade out (queue empty and notif not hidden)
        if (notifQueue.size() == 0 && !isHidden)
        {
            // trigger the animation to the notif place.
            // destination is out if the screen
            destinationX = NOTIF_WIDTH + (2 * NOTIF_OUTTER_MARGINS);
            isAnimationRunning = true;
            isHidden = true;
            animStartTime = now;
        }

        // if we need to fade in (queue not empty and notif hidden)
        else if (notifQueue.size() > 0 && isHidden)
        {
            // if so trigger the animation.
            destinationX = NOTIF_OUTTER_MARGINS;
            isAnimationRunning = true;
            animStartTime = now;
            isHidden = false;
        }
    }

    // if the box is in bound, draw it
    if (notifBaseX + popupX < bounds.getWidth())
    {
        g.setColour(COLOR_BACKGROUND);

        // draw background box
        // font size:
        g.setFont(17); // TODO: Make it an app wide param
        g.fillRoundedRectangle(notifBaseX + popupX, notifBaseY + popupY, NOTIF_WIDTH, NOTIF_HEIGHT,
                               NOTIF_BORDER_RADIUS);
        // draw text

        g.setColour(COLOR_TEXT);
        g.drawRoundedRectangle(notifBaseX + popupX, notifBaseY + popupY, NOTIF_WIDTH, NOTIF_HEIGHT, NOTIF_BORDER_RADIUS,
                               1.0);

        g.drawMultiLineText(lastNotification.message, notifBaseX + popupX + NOTIF_INNER_MARGINS,
                            notifBaseY + popupY + NOTIF_INNER_MARGINS, NOTIF_WIDTH - NOTIF_INNER_MARGINS * 2,
                            juce::Justification::left, 0.0f);
    }

    // free the lock
    queueRwMutex.exitRead();
}

void SidebarArea::trimNotifications()
{
    // get current timestamp
    uint32_t timestamp = juce::Time::getMillisecondCounter();
    // get the lock for reading the queue
    queueRwMutex.enterRead();
    // iterate over queue elements if they exists
    while (notifQueue.size() > 0)
    {
        // if it's outdated, remove it and continue
        if (notifQueue.front().timestamp + NOTIF_TIMEOUT < timestamp)
        {
            // switch from a read to a write lock
            queueRwMutex.exitRead();
            queueRwMutex.enterWrite();
            // do the queue editing
            notifQueue.pop();
            // switch from a write to a read lock
            queueRwMutex.exitWrite();
            queueRwMutex.enterRead();
            continue;
            // this branch is traversed if the oldest elem has not timed out
        }
        else
        {
            // this means other elemnts won't too so we break out of loop
            break;
        }
    }
    // free the lock for reading the queue
    queueRwMutex.exitRead();
}

void SidebarArea::resized()
{
    // this is the component bounds that we will remove area from
    // to assign it to other components
    auto remainingBounds = getLocalBounds();

    remainingBounds.reduce(SIDEBAR_OUTTER_MARGINS, 0);

    // top bar with app title and play/stop buttons
    auto topLine = remainingBounds.removeFromTop(TABS_HEIGHT);
    loopButton.setBounds(topLine.removeFromRight(SIDEBAR_ICONS_BUTTONS_WIDTH));
    topLine.removeFromRight(SIDEBAR_ICONS_BUTTONS_PADDING);
    stopButton.setBounds(topLine.removeFromRight(SIDEBAR_ICONS_BUTTONS_WIDTH));
    topLine.removeFromRight(SIDEBAR_ICONS_BUTTONS_PADDING);
    playButton.setBounds(topLine.removeFromRight(SIDEBAR_ICONS_BUTTONS_WIDTH));
    topLine.removeFromRight(SIDEBAR_ICONS_BUTTONS_PADDING);
    // the remaining area is where we draw the project title
    projectTitleArea = topLine;

    // we now proceed to add vu meters to the bottom
    auto vuMetersSection = remainingBounds.removeFromBottom(SIDEBAR_VU_METERS_AREA_HEIGHT);
    masterGainVu.setBounds(vuMetersSection.removeFromLeft(vuMetersSection.getWidth() >> 1));
    selectionGainVu.setBounds(vuMetersSection);

    // we finally start adding the main sections
    auto mainArea = remainingBounds;
    mainArea.removeFromTop(SIDEBAR_MAIN_SECTION_TOP_PADDING);
    colorPicker.setBounds(mainArea.removeFromTop(colorPicker.getIdealHeight()));
    mainArea.removeFromTop(SECTION_COMPONENTS_PADDING);
    trackProperties.setBounds(mainArea.removeFromTop(trackProperties.getIdealHeight()));
    mainArea.removeFromTop(SECTION_COMPONENTS_PADDING);
    sampleProperties.setBounds(mainArea.removeFromTop(sampleProperties.getIdealHeight()));
};

// must be called from messagemanager thread
void SidebarArea::notifyMessage(const juce::String &msg)
{
    // get a read lock
    queueRwMutex.enterRead();
    // if the queue is full ignore the notification
    if (notifQueue.size() > NOTIF_MAX_QUEUE_SIZE)
    {
        return;
    }
    // free read lock
    queueRwMutex.exitRead();

    // get current timestamp
    uint32_t timestamp = juce::Time::getMillisecondCounter();
    ;
    // create the new notification
    lastNotification.timestamp = timestamp;
    lastNotification.message = msg;

    // get the lock for writing to the queue
    queueRwMutex.enterWrite();
    // push the new notification to the queue
    notifQueue.push(lastNotification);
    // exit the writing lock
    queueRwMutex.exitWrite();

    std::cerr << "New notification at " << timestamp << ": " << msg << std::endl;

    repaint();
}

// stolen from here:
// https://stackoverflow.com/questions/13462001/ease-in-and-ease-out-animation-formula
// Implements an ease in speed modulation between 0 and 1
float SidebarArea::easeIn(float t)
{
    if (t <= 0.5f)
        return 2.0f * t * t;
    if (t <= 1.0f)
    {
        t -= 0.5f;
        return 2.0f * t * (1.0f - t) + 0.5f;
    }
    else
    {
        return 1.0;
    }
}

void SidebarArea::update()
{
}