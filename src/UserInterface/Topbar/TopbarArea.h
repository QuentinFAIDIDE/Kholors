#ifndef DEF_TOPBAR_AREA_HPP
#define DEF_TOPBAR_AREA_HPP

// CMake builds don't use an AppConfig.h, so it's safe to include juce module
// headers directly. If you need to remain compatible with Projucer-generated
// builds, and have called `juce_generate_juce_header(<thisTarget>)` in your
// CMakeLists.txt, you could `#include <JuceHeader.h>` here instead, to make all
// your module headers visible.
#include <juce_gui_extra/juce_gui_extra.h>

#include <mutex>
#include <thread>

#include "../../Arrangement/ActivityManager.h"
#include "../../Arrangement/Task.h"
#include "../LogoDarkPng.h"
#include "PlayButton.h"
#include "TopbarLeftArea.h"
#include "TopbarRightArea.h"

typedef struct
{
    uint32_t timestamp;
    juce::String message;
} NotificationMessage;

//==============================================================================
/*
    This component pops up notifications
*/
class TopbarArea : public juce::AnimatedAppComponent, public TaskListener
{
  public:
    //==============================================================================
    TopbarArea(ActivityManager &);
    ~TopbarArea();

    //==============================================================================
    // Component inherited
    bool taskHandler(std::shared_ptr<Task> task) override;
    void paint(juce::Graphics &) override;
    void paintOverChildren(juce::Graphics &) override;
    void resized() override;
    // local
    void notifyError(const juce::String &);
    // Timer inherited
    void update() override;

    /**
     * @brief      Sets the data source this VuMeter will pull from.
     *             Note that the data source actually delivers to many
     *             VuMeter based on the requested VuMeterId
     *
     * @param[in]  datasource  Instanciation of VumeterDataSource class.
     */
    void setDataSource(std::shared_ptr<MixbusDataSource>);

  private:
    //==============================================================================
    // isHidden relates to the supposed position of the animation was finished.
    // isANimationRUnning tells if the animation is currently running.
    bool isHidden, isAnimationRunning;
    std::queue<NotificationMessage> notifQueue;
    NotificationMessage lastNotification;
    // the base position of the notif box, base position does not include
    // outermargins. as we're right aligned it's the widget width minus notif box
    // width.
    int notifBaseX, notifBaseY;
    // the animation destination relative to base position
    int destinationX, destinationY;
    // Position of the popup including animation movements relative to base
    // position.
    int popupX, popupY;

    // buffer value used in the drawing function
    int maxWidth;

    // the current notif box positiooutput_addresses_ids[addr]
    // timers for animations
    int now, timeSinceAnimStart, animStartTime;
    // size and position of main content widget
    juce::Rectangle<int> bounds;
    // mutex for list updates.
    // While list appends can happen in different threads due to notifyError,
    // other variables are only used by the gui thread and should be safe to rw.
    juce::ReadWriteLock queueRwMutex;
    // a value that store total width the animation have to move in
    float animationNormalisingFactor;

    // the logo of the app we draw in left corner
    juce::Image logo;

    // left section with master and track properties
    TopbarLeftArea leftComponentsContainer;
    TopbarRightArea rightComponentsContainer;

    PlayButton playButton;

    //==============================================================================
    void trimNotifications();
    float easeIn(float t);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TopbarArea)
};

#endif // DEF_TOPBAR_AREA_HPP