#ifndef DEF_STATUS_BAR_HPP
#define DEF_STATUS_BAR_HPP

#include "juce_gui_basics/juce_gui_basics.h"

class StatusBar : public juce::Component
{
  public:
    void paint(juce::Graphics &g) override;
};

#endif // DEF_STATUS_BAR_HPP