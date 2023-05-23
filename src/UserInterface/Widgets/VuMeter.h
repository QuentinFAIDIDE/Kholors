#ifndef DEF_VU_METER_HPP
#define DEF_VU_METER_HPP

// width of the stereo vumeter
#include <memory>

#include "../../Audio/DataSource.h"

#define VUMETER_WIDTH 52
// the preferred widget width
#define VUMETER_WIDGET_WIDTH (VUMETER_WIDTH + (2 * TOPBAR_WIDGETS_MARGINS))

// padding inside the whole box
#define VUMETER_OUTTER_PADDING 2

// space beteen the squares
#define VUMETER_INNER_PADDING 1

// how many tiny little squares we define
#define VUMETER_DEFINITION 16
// how many db between each bars
#define VUMETER_SCALE_DEFINITION 3.0f
#define VUMETER_COLOR_0_MAX_DB -13.0f
#define VUMETER_COLOR_1_MAX_DB -8.0f
#define VUMETER_COLOR_2_MAX_DB -5.0f

#define COLOR_VUMETER_0 juce::Colour(114, 245, 66)
#define COLOR_VUMETER_1 juce::Colour(245, 212, 66)
#define COLOR_VUMETER_2 juce::Colour(245, 138, 66)
#define COLOR_VUMETER_3 juce::Colour(245, 72, 66)

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
    VuMeter(std::string, VumeterId);

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

    /**
     * @brief      Sets the data source this VuMeter will pull from.
     *             Note that the data source actually delivers to many
     *             VuMeter based on the requested VuMeterId
     *
     * @param[in]  datasource  Instanciation of VumeterDataSource class.
     */
    void setDataSource(std::shared_ptr<VuMeterDataSource>);

  private:
    // the actual decibels values of left and righht channel
    float dbValueLeft, dbValueRight;

    // the title of the vu meter
    std::string title;

    // the enumeration identifying the data source of the vu meter
    // in the Mixbus data source.
    VumeterId identifier;

    // datasource pointer
    std::shared_ptr<VuMeterDataSource> dataSource;

    ///////////////////////////////

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

    /**
     Draw the channel vu meter area. It's made of VUMETER_DEFINITION rectangles of 4 colors.
     */
    void drawChannel(juce::Graphics &, juce::Rectangle<int>, float);

    /**
     Pick a color that corresponds to the index in the range.
     The scale is described the the environement variables for
     the colors and thresolds. It will also make the color dark
     if above the db value sent.
     */
    juce::Colour pickColorForIndex(int, int, float);

    /**
     Updates the value of the vu meter through the mixbus data source.
     Might fail to get the lock and do nothing.
     */
    void updateValue();
};

#endif // DEF_VU_METER_HPP