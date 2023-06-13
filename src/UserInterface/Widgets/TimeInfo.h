#ifndef DEF_TIME_INFO_HPP
#define DEF_TIME_INFO_HPP

#include <juce_gui_basics/juce_gui_basics.h>

#include "../CustomFonts.h"

#define TIMEINFO_TEXT_MARGINS 3
#define TIMEINFO_WIDTH 95

class TimeInfo : public juce::Component
{
  public:
    TimeInfo();
    void paint(juce::Graphics &g) override;

  private:
    int frameValue;
    juce::SharedResourcePointer<CustomFonts> sharedFonts;
};

#endif // DEF_TIME_INFO_HPP