#include "NotificationArea.h"

NotificationArea::NotificationArea() {
  isHidden = true;
  isAnimationRunning = true;
  popupY = NOTIF_OUTTER_MARGINS;
  popupX = 0;
  firstPaint = true;
}

NotificationArea::~NotificationArea() {}

void NotificationArea::paint(juce::Graphics& g) {

  // get the window width
  bounds = g.getClipBounds();

  // if we have never painted, get bound and set notif box position
  if(firstPaint) {
    firstPaint=false;
    popupX = bounds.getWidth() + NOTIF_OUTTER_MARGINS;
  }

  now = juce::Time::getMillisecondCounter();
  // make sure the notification are filetered to remove old ones
  trimNotifications();
  // if we are currently performing an animation
  if (isAnimationRunning) {
    timeSinceLastPaint = now - lastPaintTime;
    lastPaintTime = now;
    // continue it
    // TODO: add a formula to distance to smooth animation in and out
    // if it's going out go left
    if (isHidden == true) {
      popupX += NOTIF_ANIMATION_SPEED * (timeSinceLastPaint);
      // if it reached destination, stop it
      if (popupX >= destinationX) {
        popupX = destinationX;
        isAnimationRunning = false;
      }
      // if it's going in go right
    } else {
      popupX -= NOTIF_ANIMATION_SPEED * (timeSinceLastPaint);
      // if it reached destination, stop it
      if (popupX <= destinationX) {
        popupX = destinationX;
        isAnimationRunning = false;
      }
    }
    // if we are not performing an animation
  } else {
    // check if we need to fade out
    if (notifQueue.size() == 0 && !isHidden) {
      // if so trigger the animation
      destinationX = bounds.getWidth()+NOTIF_OUTTER_MARGINS;
      isAnimationRunning = true;
      lastPaintTime = juce::Time::getMillisecondCounter();
    }
    // check if we need to fade in
    if (notifQueue.size() > 0 && isHidden) {
      // if so trigger the animation
      destinationX = bounds.getWidth() - NOTIF_WIDTH - NOTIF_OUTTER_MARGINS;
      isAnimationRunning = true;
      lastPaintTime = juce::Time::getMillisecondCounter();
    }
  }
  // if the box is in bound, draw it
  if(popupX < bounds.getWidth()) {
    g.setColour(COLOR_NOTIF_BACKGROUND);
    g.fillRoundedRectangle (popupX, popupY, NOTIF_WIDTH, NOTIF_HEIGHT, NOTIF_BORDER_RADIUS);
    // TODO: change font
    g.drawMultiLineText	(lastNotification.message,
        popupX + NOTIF_INNER_MARGINS,
        popupY + NOTIF_INNER_MARGINS,
        popupX + NOTIF_WIDTH - NOTIF_INNER_MARGINS,
        juce::Justification::left,
        0.0f 
    );
  }
}

void NotificationArea::trimNotifications() {
  // get current timestamp
  int32_t timestamp = juce::Time::getMillisecondCounter();
  // for each notification in the reversed order
  int i = 0;
  while (i == 0) {
    // if it's outdated, remove it and continue
    if (notifQueue.front().timestamp + NOTIF_TIMEOUT > timestamp) {
      notifQueue.pop();
      continue;
    }
    i++;
  }
}

void NotificationArea::resized() {};

void NotificationArea::mouseDown(const juce::MouseEvent& me) {}

void NotificationArea::mouseUp(const juce::MouseEvent& me) {}

void NotificationArea::mouseDrag(const juce::MouseEvent& me) {}

void NotificationArea::mouseMove(const juce::MouseEvent& me) {}

void NotificationArea::notifyError(const juce::String& msg) {
  // get current timestamp
  int32_t timestamp = juce::Time::getMillisecondCounter();;
  // create the new notification
  lastNotification.timestamp = timestamp;
  lastNotification.message = msg;
  // add it
  notifQueue.push(lastNotification);
}