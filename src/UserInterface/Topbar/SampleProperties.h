#ifndef DEF_SAMPLE_PROPERTIES_HPP
#define DEF_SAMPLE_PROPERTIES_HPP

#include "../../Arrangement/ActivityManager.h"
#include <juce_gui_extra/juce_gui_extra.h>

class SampleProperties : public juce::Component
{
  public:
    SampleProperties(ActivityManager &am);
    void paint(juce::Graphics &g) override;
    void resized() override;
};

#endif // DEF_SAMPLE_PROPERTIES_HPP