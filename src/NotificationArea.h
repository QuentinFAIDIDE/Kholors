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
class NotificationArea : public juce::AnimatedAppComponent {
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
  void update() override;

 private:
  //==============================================================================
  // isHidden relates to the supposed position of the animation was finished.
  // isANimationRUnning tells if the animation is currently running.
  bool isHidden, isAnimationRunning;
  std::queue<NotificationMessage> notifQueue;
  NotificationMessage lastNotification;
  // the base position of the notif box, base position does not include outermargins.
  // as we're right aligned it's the widget width minus notif box width. 
  int baseX, baseY;
  // the animation destination relative to base position
  int destinationX, destinationY;
  // Position of the popup including animation movements relative to base position.
  int popupX, popupY;

  // part in brackets is the actual drawn card when it's not animated:

  //   left outer margin  baseX + OUTER_MARGINS                            right outer margins
  //               |      |                                                     |
  //     baseX     |      |          baseX + OUTER_MARGINS + INNER_MARGINS      |
  //     |         |      |          |                                          |
  //     |                [----------| My notfication text here |----------]        )   
  //                           |                                      |             |
  //                    left inner margin                    right inner margin     |
  //                                                                                screen end

  // buffer value used in the drawing function
  int maxWidth;

  // the current notif box positiooutput_addresses_ids[addr] 
  // timers for animations
  int now, timeSinceAnimStart, animStartTime;
  // size and position of main content widget
  juce::Rectangle<int> bounds;
  // mutex for list updates
  std::mutex notifMutex;

  float animationNormalisingFactor;

  //==============================================================================
  void trimNotifications();
  float easeIn(float t);
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NotificationArea)
};

#endif  // DEF_NOTIFICATIONAREA_HPP