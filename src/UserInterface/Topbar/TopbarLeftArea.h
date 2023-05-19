#ifndef DEF_TOPBAR_LEFT_AREA
#define DEF_TOPBAR_LEFT_AREA

#include "../../Config.h"
#include "../Widgets/VuMeter.h"

#include <juce_gui_extra/juce_gui_extra.h>

class TopbarLeftArea : public juce::Component
{
  public:
    TopbarLeftArea();
    void paint(juce::Graphics &) override;
    void resized() override;

  private:
    // the outer bounds, minus the LeftArea inner margins.
    juce::Rectangle<float> bounds;
    VuMeter masterGainVu;
    VuMeter inputGainVu;
};

#endif // DEF_TOPBAR_LEFT_AREA