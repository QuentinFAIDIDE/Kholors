#ifndef DEF_TOPBAR_RIGHT_AREA
#define DEF_TOPBAR_RIGHT_AREA

#include "../../Config.h"
#include "ColorPicker.h"

#include <juce_gui_extra/juce_gui_extra.h>

class TopbarRightArea : public juce::Component
{
  public:
    TopbarRightArea(ActivityManager &);
    void paint(juce::Graphics &) override;
    void resized() override;

  private:
    // the outer bounds, minus the inner margins.
    juce::Rectangle<float> bounds;
    ColorPicker colorPicker;
};

#endif // DEF_TOPBAR_RIGHT_ARE