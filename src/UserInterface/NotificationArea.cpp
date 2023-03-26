#include "NotificationArea.h"

NotificationArea::NotificationArea() {
  isHidden = true;
  isAnimationRunning = false;
  baseX = 0;
  baseY = 0;
  popupY = NOTIF_OUTTER_MARGINS;
  popupX = NOTIF_WIDTH + (2 * NOTIF_OUTTER_MARGINS);
  setFramesPerSecond(NOTIF_ANIM_FPS);
  animationNormalisingFactor =
      float(NOTIF_WIDTH + (2.0 * NOTIF_OUTTER_MARGINS));
  _logo = juce::ImageFileFormat::loadFrom(LogoDarkPng::logo_dark_png,
                                          LogoDarkPng::logo_dark_pngSize);
}

NotificationArea::~NotificationArea() {}

void NotificationArea::paint(juce::Graphics& g) {
  // make sure the notification are filetered to remove old ones
  trimNotifications();

  // if we can't get a read lock, skip any action
  if (!queueRwMutex.tryEnterRead()) {
    return;
  }

  // TODO: do nothing whenever nothing changed
  // and nothing requires anything to start or end.

  // get the window width
  bounds = g.getClipBounds();
  baseX = bounds.getWidth() - 2 * NOTIF_OUTTER_MARGINS - NOTIF_WIDTH;

  g.setColour(COLOR_APP_BACKGROUND);
  g.fillAll();

  // draw background over bounds
  // paint the background of the area
  g.setColour(juce::Colour(20, 20, 20));
  juce::Rectangle<float> innerArea(6, 6, bounds.getWidth() - 12,
                                   bounds.getHeight() - 12);
  g.fillRoundedRectangle(innerArea, 4);
  g.setColour(juce::Colour(200, 200, 200));
  g.drawRoundedRectangle(innerArea, 4, 0.4);

  g.drawImageAt(_logo, 14, 14);

  // get time of now to reuse it in the animation drawings parts
  now = juce::Time::getMillisecondCounter();

  // if we are currently performing an animation
  if (isAnimationRunning) {
    timeSinceAnimStart = now - animStartTime;

    // TODO: add a formula of distance to smooth animation in and out.

    // the distance shift from the beginning of the anim
    int distanceDiff = int(
        // make it over 1 to use the ease in function
        easeIn((NOTIF_ANIMATION_SPEED_PIXEL_MS * float(timeSinceAnimStart)) /
               animationNormalisingFactor)
        // scale it back up after applying ease in function
        * animationNormalisingFactor);

    // if it's going out go left
    if (isHidden == true) {
      popupX = NOTIF_OUTTER_MARGINS + distanceDiff;
      // if it reached destination, stop it
      if (popupX >= destinationX) {
        popupX = destinationX;
        isAnimationRunning = false;
      }

      // if it's going in go right
    } else {
      // new position is the screen limit minus the notif movement toward
      // center. we make the fade in faster than fade out for aesthetic reasons
      popupX = NOTIF_WIDTH + (2 * NOTIF_OUTTER_MARGINS) -
               int(float(distanceDiff) * 4.0);
      // if it reached destination, stop it
      if (popupX <= destinationX) {
        popupX = destinationX;
        isAnimationRunning = false;
      }
    }

    // if we are not performing an animation
  } else {
    // if we need to fade out (queue empty and notif not hidden)
    if (notifQueue.size() == 0 && !isHidden) {
      // trigger the animation to the notif place.
      // destination is out if the screen
      destinationX = NOTIF_WIDTH + (2 * NOTIF_OUTTER_MARGINS);
      isAnimationRunning = true;
      isHidden = true;
      animStartTime = now;
    }

    // if we need to fade in (queue not empty and notif hidden)
    else if (notifQueue.size() > 0 && isHidden) {
      // if so trigger the animation.
      destinationX = NOTIF_OUTTER_MARGINS;
      isAnimationRunning = true;
      animStartTime = now;
      isHidden = false;
    }
  }

  // if the box is in bound, draw it
  if (baseX + popupX < bounds.getWidth()) {
    g.setColour(COLOR_NOTIF_BACKGROUND);

    // draw background box
    // font size:
    g.setFont(17);  // TODO: Make it an app wide param
    g.fillRoundedRectangle(baseX + popupX, baseY + popupY, NOTIF_WIDTH,
                           NOTIF_HEIGHT, NOTIF_BORDER_RADIUS);
    // draw text

    g.setColour(COLOR_NOTIF_TEXT);
    g.drawRoundedRectangle(baseX + popupX, baseY + popupY, NOTIF_WIDTH,
                           NOTIF_HEIGHT, NOTIF_BORDER_RADIUS, 1.0);

    g.drawMultiLineText(
        lastNotification.message, baseX + popupX + NOTIF_INNER_MARGINS,
        baseY + popupY + NOTIF_INNER_MARGINS,
        NOTIF_WIDTH - NOTIF_INNER_MARGINS * 2, juce::Justification::left, 0.0f);
  }

  // free the lock
  queueRwMutex.exitRead();
}

void NotificationArea::trimNotifications() {
  // get current timestamp
  uint32_t timestamp = juce::Time::getMillisecondCounter();
  // get the lock for reading the queue
  queueRwMutex.enterRead();
  // iterate over queue elements if they exists
  while (notifQueue.size() > 0) {
    // if it's outdated, remove it and continue
    if (notifQueue.front().timestamp + NOTIF_TIMEOUT < timestamp) {
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
    } else {
      // this means other elemnts won't too so we break out of loop
      break;
    }
  }
  // free the lock for reading the queue
  queueRwMutex.exitRead();
}

void NotificationArea::resized(){};

void NotificationArea::mouseDown(const juce::MouseEvent& me) {}

void NotificationArea::mouseUp(const juce::MouseEvent& me) {}

void NotificationArea::mouseDrag(const juce::MouseEvent& me) {}

void NotificationArea::mouseMove(const juce::MouseEvent& me) {}

void NotificationArea::notifyError(const juce::String& msg) {
  // get a read lock
  queueRwMutex.enterRead();
  // if the queue is full ignore the notification
  if (notifQueue.size() > NOTIF_MAX_QUEUE_SIZE) {
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

  std::cerr << "New error notification at " << timestamp << ": " << msg
            << std::endl;

  // we will let the repaint function starts the animation
  // call a repaint to get the anim started.
  // As it's a component function, we need to get the message manager lock.
  const juce::MessageManagerLock mmLock;
  repaint();
}

// stolen from here:
// https://stackoverflow.com/questions/13462001/ease-in-and-ease-out-animation-formula
// Implements an ease in speed modulation between 0 and 1
float NotificationArea::easeIn(float t) {
  if (t <= 0.5f) return 2.0f * t * t;
  if (t <= 1.0f) {
    t -= 0.5f;
    return 2.0f * t * (1.0f - t) + 0.5f;
  } else {
    return 1.0;
  }
}

void NotificationArea::update() {}