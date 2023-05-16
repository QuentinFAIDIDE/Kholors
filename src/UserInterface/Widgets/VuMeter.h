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

class VuMeter : public juce::Component
{
  public:
    VuMeter(std::string);
    void paint(juce::Graphics &g) override;
    void mouseDrag(const juce::MouseEvent &event) override;
    void setDbValue(float, float);

  private:
    float dbValueLeft, dbValueRight;
    float dbMaxLeft, dbMaxRight;
    std::string title;
};

#endif // DEF_VU_METER_HPP