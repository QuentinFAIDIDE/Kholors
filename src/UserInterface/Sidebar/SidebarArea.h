#ifndef DEF_SIDEBAR_AREA_HPP
#define DEF_SIDEBAR_AREA_HPP

#include <juce_gui_extra/juce_gui_extra.h>

#include <memory>
#include <mutex>
#include <thread>

#include "../../Arrangement/ActivityManager.h"
#include "../../Arrangement/Task.h"
#include "../LogoDarkPng.h"
#include "../Widgets/VuMeter.h"
#include "ColorPicker.h"
#include "LoopButton.h"
#include "SampleProperties.h"
#include "StopButton.h"
#include "TrackProperties.h"

#define SIDEBAR_WIDTH 226
#define SIDEBAR_VU_METERS_AREA_HEIGHT 235
#define SIDEBAR_COLOR_PICKER_HEIGHT 90
#define SIDEBAR_TRACK_PROPERTIES_HEIGHT (22 + LABELED_LINE_CONTAINER_DEFAULT_HEIGHT * 3)
#define SIDEBAR_SAMPLE_PROPERTIES_HEIGHT (22 + LABELED_LINE_CONTAINER_DEFAULT_HEIGHT * 5)
#define SIDEBAR_MAIN_SECTION_TOP_PADDING 4
typedef struct
{
    uint32_t timestamp;
    juce::String message;
} NotificationMessage;

//==============================================================================
/*
    This component pops up notifications
*/
class SidebarArea : public juce::AnimatedAppComponent, public TaskListener
{
  public:
    //==============================================================================
    SidebarArea(ActivityManager &);
    ~SidebarArea();

    //==============================================================================
    // Component inherited
    bool taskHandler(std::shared_ptr<Task> task) override;
    void paint(juce::Graphics &) override;
    void paintOverChildren(juce::Graphics &) override;
    void resized() override;
    // local
    void notifyMessage(const juce::String &);
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
    juce::Rectangle<int> projectTitleArea;
    // mutex for list updates.
    // While list appends can happen in different threads due to notifyError,
    // other variables are only used by the gui thread and should be safe to rw.
    juce::ReadWriteLock queueRwMutex;
    // a value that store total width the animation have to move in
    float animationNormalisingFactor;

    PlayButton playButton;
    StopButton stopButton;
    LoopButton loopButton;

    ColorPicker colorPicker;
    TrackProperties trackProperties;
    SampleProperties sampleProperties;

    VuMeter selectionGainVu;
    VuMeter masterGainVu;

    //==============================================================================
    void trimNotifications();
    float easeIn(float t);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SidebarArea)
};

#endif // DEF_SIDEBAR_AREA_HPP