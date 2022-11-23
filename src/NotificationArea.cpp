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
    popupX = bounds.getWidth + NOTIF_OUTTER_MARGINS;
  }

  now = juce::Time::getMillisecondCounter();
  // make sure the notification are filetered to remove old ones
  trimNotifications();
  // if we are currently performing an animation
  if (isAnimationRunning) {
    timeSinceLastPaint = now - m_lastPaintTime;
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
    }
    // check if we need to fade in
    if (notifQueue.size() > 0 && isHidden) {
      // if so trigger the animation
      destinationX = bounds.getWidth() - NOTIF_WIDTH - NOTIF_OUTTER_MARGINS;
      isAnimationRunning = true;
    }
  }
  // if the box is in bound, draw it
  if(popupX < bounds.getWidth()) {
    g.setColour(COLOR_NOTIF_BACKGROUND);
    g.fillRoundedRectangle (popupX, popupY, NOTIF_WIDTH, NOTIF_HEIGHT, NOTIF_BORDER_RADIUS);
    // TODO: change font
    g.drawMultiLineText	(lastNotification,
        int 	startX,
        int 	baselineY,
        int 	maximumLineWidth,
        Justification 	justification = Justification::left,
        float 	leading = 0.0f 
        );
  }
}

void NotificationArea::trimNotifications() {
  // get current timestamp
  int64_t timestamp =
      duration_cast<milliseconds>(system_clock::now().time_since_epoch())
          .count();
  // for each notification in the reversed order
  int i = 0;
  while (i == 0) {
    // if it's outdated, remove it and continue
    if (notifQueue[i].timestamp + NOTIFICATION_TIMEOUT > timestamp) {
      notifQueue.pop_front();
      continue;
    }
    i++;
  }
}

void NotificationArea::mouseDown(const juce::MouseEvent& me) {}

void NotificationArea::mouseUp(const juce::MouseEvent& me) {}

void NotificationArea::mouseDrag(const juce::MouseEvent& me) {}

void NotificationArea::mouseMove(const juce::MouseEvent& me) {}

void NotificationArea::notifyError(const juce::String& msg) {
  // get current timestamp
  int64_t timestamp =
      duration_cast<milliseconds>(system_clock::now().time_since_epoch())
          .count();
  // create the new notification
  lastNotification.timestamp = timestamp;
  lastNotification.message = msg;
  // add it
  notifQueue.push_back(newNotif);
  // last notification
  lastNotification = newNotif;
}