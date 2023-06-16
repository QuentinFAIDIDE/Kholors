#include "TopbarArea.h"

TopbarArea::TopbarArea(ActivityManager &am) : rightComponentsContainer(am)
{
    isHidden = true;
    isAnimationRunning = false;
    notifBaseX = 0;
    notifBaseY = 0;
    popupY = NOTIF_OUTTER_MARGINS;
    popupX = NOTIF_WIDTH + (2 * NOTIF_OUTTER_MARGINS);
    setFramesPerSecond(NOTIF_ANIM_FPS);
    animationNormalisingFactor = float(NOTIF_WIDTH + (2.0 * NOTIF_OUTTER_MARGINS));
    logo = juce::ImageFileFormat::loadFrom(LogoDarkPng::logo_dark_png, LogoDarkPng::logo_dark_pngSize);

    addAndMakeVisible(leftComponentsContainer);
    addAndMakeVisible(rightComponentsContainer);
}

TopbarArea::~TopbarArea()
{
}

void TopbarArea::setDataSource(std::shared_ptr<MixbusDataSource> ds)
{
    leftComponentsContainer.setDataSource(ds);
    rightComponentsContainer.setDataSource(ds);
}

bool TopbarArea::taskHandler(std::shared_ptr<Task> task)
{
    std::shared_ptr<NotificationTask> notif = std::dynamic_pointer_cast<NotificationTask>(task);

    if (notif != nullptr && !notif->isCompleted() && !notif->hasFailed())
    {
        notifyError(notif->getMessage());
        return true;
    }

    return false;
}

void TopbarArea::paint(juce::Graphics &g)
{
    g.setColour(COLOR_BACKGROUND);
    g.fillAll();

    // we then draw a card at half the outter margins
    juce::Rectangle<float> cardInsideArea =
        bounds.reduced(TOPBAR_OUTTER_MARGINS >> 1, TOPBAR_OUTTER_MARGINS >> 1).toFloat();
    g.setColour(COLOR_BACKGROUND_LIGHTER);
    g.fillRoundedRectangle(cardInsideArea, TOPBAR_BORDER_RADIUS);
    g.setColour(COLOR_LABELS_BORDER);
    g.drawRoundedRectangle(cardInsideArea, TOPBAR_BORDER_RADIUS, 0.4);

    int imageY = (bounds.getHeight() / 2.0) - (logo.getHeight() / 2.0);

    g.drawImageAt(logo, TOPBAR_OUTTER_MARGINS, imageY);
}

void TopbarArea::paintOverChildren(juce::Graphics &g)
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
    now = juce::Time::getMillisecondCounter();

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

void TopbarArea::trimNotifications()
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

void TopbarArea::resized()
{
    auto area = getLocalBounds();
    area.reduce(TOPBAR_OUTTER_MARGINS, TOPBAR_OUTTER_MARGINS);
    auto leftSection = area.removeFromLeft(TOPBAR_LEFT_SECTION_WIDTH);
    leftSection.removeFromLeft(logo.getWidth());
    leftComponentsContainer.setBounds(leftSection);

    area = getLocalBounds();
    area.reduce(TOPBAR_OUTTER_MARGINS, TOPBAR_OUTTER_MARGINS);
    auto rightSection = area.removeFromRight(TOPBAR_RIGHT_SECTION_WIDTH);
    rightComponentsContainer.setBounds(rightSection);
};

void TopbarArea::mouseDown(const juce::MouseEvent &me)
{
}

void TopbarArea::mouseUp(const juce::MouseEvent &me)
{
}

void TopbarArea::mouseDrag(const juce::MouseEvent &me)
{
}

void TopbarArea::mouseMove(const juce::MouseEvent &me)
{
}

// must be called from messagemanager thread
void TopbarArea::notifyError(const juce::String &msg)
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

    std::cerr << "New error notification at " << timestamp << ": " << msg << std::endl;

    repaint();
}

// stolen from here:
// https://stackoverflow.com/questions/13462001/ease-in-and-ease-out-animation-formula
// Implements an ease in speed modulation between 0 and 1
float TopbarArea::easeIn(float t)
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

void TopbarArea::update()
{
}