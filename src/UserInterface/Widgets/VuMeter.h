#ifndef DEF_VU_METER_HPP
#define DEF_VU_METER_HPP

// width of the stereo vumeter
#define VUMETER_WIDTH 52
// the preferred widget width
#define VUMETER_WIDGET_WIDTH (VUMETER_WIDTH + (2 * TOPBAR_WIDGETS_MARGINS))

// padding inside the whole box
#define VUMETER_OUTTER_PADDING 2

// space beteen the squares
#define VUMETER_INNER_PADDING 1

// how many tiny little squares we define
#define VUMETER_DEFINITION 32
// how many db between each bars
#define VUMETER_SCALE_DEFINITION 3.0
// minimum db value displayed
#define VUMETER_MIN_DB -24.0

#include <juce_gui_basics/juce_gui_basics.h>

/*
Displays a stereo VuMeter horizontal vu meter with
title and max values at the bottom.
*/
class VuMeter : public juce::Component
{
  public:
    /*
    Initialize the vumeter with the given title and identifier.
    */
    VuMeter(std::string, std::string);

    /*
    paint the vu meter
    */
    void paint(juce::Graphics &g) override;

    /*
    Responds to mouse drag events.
    */
    void mouseDrag(const juce::MouseEvent &event) override;

    /*
    sets the decibel values for left and right
    */
    void setDbValue(float, float);

  private:
    // the actual decibels values of left and righht channel
    float dbValueLeft, dbValueRight;

    // the title of the vu meter
    std::string title;

    // the string identifying the data source of the vu meter
    // in the Mixbus data source.
    std::string identifier;

    /*
    Return the area within the section at the desired vu meter width.
    (No title and margins)
    */
    juce::Rectangle<int> zoomToInnerSection(juce::Rectangle<int>);

    /*
    Paint the core visualizer part of the vu meter
    */
    void paintCoreMeter(juce::Graphics &, juce::Rectangle<int>);

    /**
    Paint the scale area on the right where the grading and number lives.
    */
    void drawScale(juce::Graphics &g, juce::Rectangle<int>);

    /**
     Paint the vu area where the left and right channel are displayed.
     */
    void drawMeter(juce::Graphics &g, juce::Rectangle<int>);
};

#endif // DEF_VU_METER_HPP