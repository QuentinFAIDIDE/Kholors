#ifndef DEF_NOTIFICATIONAREA_HPP
#define DEF_NOTIFICATIONAREA_HPP

// CMake builds don't use an AppConfig.h, so it's safe to include juce module
// headers directly. If you need to remain compatible with Projucer-generated
// builds, and have called `juce_generate_juce_header(<thisTarget>)` in your
// CMakeLists.txt, you could `#include <JuceHeader.h>` here instead, to make all
// your module headers visible.
#include <juce_gui_extra/juce_gui_extra.h>

#include <thread>
#include <mutex>

#include "Config.h"

typedef struct {
  int32_t timestamp;
  juce::String message;
} NotificationMessage;

//==============================================================================
/*
    This component pops up notifications
*/
class NotificationArea : public juce::Component, private juce::Timer {
 public:
  //==============================================================================
  NotificationArea();
  ~NotificationArea();

  //==============================================================================
  // Component inherited
  void paint(juce::Graphics&) override;
  void resized() override;
  void mouseDown(const juce::MouseEvent&) override;
  void mouseUp(const juce::MouseEvent&) override;
  void mouseDrag(const juce::MouseEvent&) override;
  void mouseMove(const juce::MouseEvent&) override;
  // local
  void notifyError(const juce::String&);
  // Timer inherited
  void timerCallback() override;

 private:
  //==============================================================================
  bool isHidden, isAnimationRunning;
  std::queue<NotificationMessage> notifQueue;
  NotificationMessage lastNotification;
  // the directirion of the notification
  int destinationX, destinationY;
  // the current notif box position
  int popupX, popupY;
  // timers for animations
  int now, timeSinceLastPaint, lastPaintTime;
  // size and position of main content widget
  juce::Rectangle<int> bounds;
  // have we been painting already ?
  bool firstPaint;
  // mutex for list updates
  std::mutex notifMutex;
  //==============================================================================
  void trimNotifications();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NotificationArea)
};

#endif  // DEF_NOTIFICATIONAREA_HPP