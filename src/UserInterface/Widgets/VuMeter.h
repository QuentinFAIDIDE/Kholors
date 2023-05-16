#ifndef DEF_VU_METER_HPP
#define DEF_VU_METER_HPP

// width of the stereo vumeter
#define VUMETER_WIDTH 60
// height of the area where max db are displayed
#define VUMETER_MAXVAL_HEIGHT 14
// how many tiny little squares we define
#define VUMETER_DEFINITION 32
// minimum db value displayed
#define VUMETER_MIN_DB -60.0

#include <juce_gui_basics/juce_gui_basics.h>

/*
Displays a stereo VuMeter horizontal vu meter with
title and max values at the bottom.
*/
class VuMeter : public juce::Component
{
  public:
    /*
    Initialize the vumeter with the given title.
    */
    VuMeter(std::string);

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

    // the maximum values we've seen in a while
    float dbMaxLeft, dbMaxRight;

    // the title of the vu meter
    std::string title;

    /*
    Return the area within the section at the desired vu meter width.
    (No title and margins)
    */
    void zoomToInnerSection(juce::Rectangle<int>);

    /*
    Paint the core visualizer part of the vu meter
    */
    void paintCoreMeter(juce::Graphics &, juce::Rectangle<int>);

    /*
    Paint the max db values information below the visualizer.
    */
    void paintBottomValues(juce::Graphics &, juce::Rectangle<int>);
};

#endif // DEF_VU_METER_HPP