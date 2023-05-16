#ifndef DEF_VU_METER_HPP
#define DEF_VU_METER_HPP

#define VUMETER_WIDTH 60

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
    std::string title;
};

#endif // DEF_VU_METER_HPP