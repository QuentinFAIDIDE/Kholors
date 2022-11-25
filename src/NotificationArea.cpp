#include "NotificationArea.h"

NotificationArea::NotificationArea() {
  isHidden = true;
  isAnimationRunning = false;
  baseX = 0;
  baseY = 0;
  popupY = NOTIF_OUTTER_MARGINS;
  popupX = NOTIF_WIDTH + (2 * NOTIF_OUTTER_MARGINS);
  setFramesPerSecond(25);
}

NotificationArea::~NotificationArea() {}

void NotificationArea::paint(juce::Graphics& g) {

  // TODO: do nothing whenever nothing changed
  // and nothing requires anything to start or end.

  // get the window width
  bounds = g.getClipBounds();
  baseX = bounds.getWidth() - 2*NOTIF_OUTTER_MARGINS - NOTIF_WIDTH;

  // draw background over bounds
  // paint the background of the area
  g.setColour(COLOR_APP_BACKGROUND);
  g.fillRect(bounds);

  // get time of now to reuse it in the animation drawings parts
  now = juce::Time::getMillisecondCounter();

  // make sure the notification are filetered to remove old ones
  trimNotifications();

  std::cout << "repainting" << std::endl;
  std::cout << "isHidden:" << isHidden << std::endl;
  std::cout << "isAnimationRunning:" << isAnimationRunning << std::endl;
  std::cout << "popupX:" << popupX << std::endl;
  std::cout << "popupY:" << popupY << std::endl;
  std::cout << "notifQueue.size():" << notifQueue.size() << std::endl;

  // if we are currently performing an animation
  if (isAnimationRunning) {
    timeSinceAnimStart = now - animStartTime;

    // TODO: add a formula of distance to smooth animation in and out.

    int distanceDiff = int(NOTIF_ANIMATION_SPEED_PIXEL_MS * float(timeSinceAnimStart));

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
      // new position is the screen limit minus the notif movement toward center
      popupX = NOTIF_WIDTH + (2*NOTIF_OUTTER_MARGINS) - distanceDiff;
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
      destinationX = NOTIF_WIDTH + (2*NOTIF_OUTTER_MARGINS);
      isAnimationRunning = true;
      isHidden = true;
      animStartTime = now;
      std::cout << "startanim1" << std::endl;
    }

    // if we need to fade in (queue not empty and notif hidden)
    else if (notifQueue.size() > 0 && isHidden) {
      // if so trigger the animation.
      destinationX = 0;
      isAnimationRunning = true;
      animStartTime = now;
      isHidden = false;
      std::cout << "startanim2" << std::endl;
    }
  }

  // if the box is in bound, draw it
  if(baseX + popupX + NOTIF_WIDTH < bounds.getWidth()) {
    g.setColour(COLOR_NOTIF_BACKGROUND);
    // draw background box
    g.fillRoundedRectangle(
      baseX + popupX,
      baseY + popupY,
      NOTIF_WIDTH,
      NOTIF_HEIGHT,
      NOTIF_BORDER_RADIUS
    );
    // draw text
    // TODO: change font

    g.setColour(COLOR_NOTIF_TEXT);
    g.drawMultiLineText	(lastNotification.message,
        baseX + popupX + NOTIF_INNER_MARGINS,
        baseY + popupY + NOTIF_INNER_MARGINS,
        baseX + popupX + NOTIF_WIDTH - NOTIF_INNER_MARGINS*2,
        juce::Justification::left,
        0.0f 
    );
  }
}

void NotificationArea::trimNotifications() {
  // get current timestamp
  int32_t timestamp = juce::Time::getMillisecondCounter();
  // iterate over queue elements if they exists
  while (notifQueue.size()>0) {
    // if it's outdated, remove it and continue
    if (notifQueue.front().timestamp + NOTIF_TIMEOUT < timestamp) {
      notifQueue.pop();
      continue;
    // this branch is traversed if the oldest elem has not timed out
    } else {
      // this means other elemnts won't too so we break out of loop
      break;
    }
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
  // we will let the repaint function starts the animation
  // call a repaint to get the anim started
  std::cout << "New error notification at " << timestamp << ": " << msg << std::endl;
  repaint();
}

void NotificationArea::update() {
  std::cout << "timer called" << std::endl;
  //repaint();
}