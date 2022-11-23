#ifndef DEF_NOTIFICATIONAREA_HPP
#define DEF_NOTIFICATIONAREA_HPP

// CMake builds don't use an AppConfig.h, so it's safe to include juce module
// headers directly. If you need to remain compatible with Projucer-generated
// builds, and have called `juce_generate_juce_header(<thisTarget>)` in your
// CMakeLists.txt, you could `#include <JuceHeader.h>` here instead, to make all
// your module headers visible.
#include <juce_gui_extra/juce_gui_extra.h>

#include <chrono>

#include "Config.h"

typedef struct {
  int64_t timestamp;
  juce::String message;
} NotificationMessage;

//==============================================================================
/*
    This component pops up notifications
*/
class NotificationArea : public juce::Component {
 public:
  //==============================================================================
  NotificationArea();
  ~NotificationArea();

  //==============================================================================
  void paint(juce::Graphics&) override;
  void resized() override;
  void mouseDown(const juce::MouseEvent&) override;
  void mouseUp(const juce::MouseEvent&) override;
  void mouseDrag(const juce::MouseEvent&) override;
  void mouseMove(const juce::MouseEvent&) override;
  void notifyError(juce::String&);

 private:
  //==============================================================================
  bool isHidden, isAnimationRunning;
  std::vector<NotificationMessage> notifQueue;
  NotificationMessage lastNotification;
  // the directirion of the notification
  int destinationX, destinationY;
  // the current notif box position
  int popupX, popupY;
  // timers for animations
  int now, timeSinceLastPaint;
  // size and position of main content widget
  juce::Rectangle<int> bounds;
  // have we been painting already ?
  bool firstPaint;
  //==============================================================================
  void trimNotifications();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NotificationArea)
};

#endif  // DEF_NOTIFICATIONAREA_HPP