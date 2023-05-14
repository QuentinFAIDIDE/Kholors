#ifndef DEF_LOOKANDFEEL_HPP
#define DEF_LOOKANDFEEL_HPP

#include <juce_gui_extra/juce_gui_extra.h>

class KholorsLookAndFeel : public juce::LookAndFeel_V4
{
  public:
    KholorsLookAndFeel();
    void drawTabButton(juce::TabBarButton &, juce::Graphics &, bool isMouseOver, bool isMouseDown) override;
};

#endif // DEF_LOOKANDFEEL_HPP