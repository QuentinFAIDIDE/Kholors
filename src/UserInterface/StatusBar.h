#ifndef DEF_STATUS_BAR_HPP
#define DEF_STATUS_BAR_HPP

#include "StatusTips.h"
#include "juce_gui_basics/juce_gui_basics.h"

class StatusBar : public juce::Component
{
  public:
    StatusBar();
    ~StatusBar();
    void paint(juce::Graphics &g) override;

  private:
    juce::SharedResourcePointer<StatusTips> sharedStatusTips;
    std::string lastPositionTip;
};

#endif // DEF_STATUS_BAR_HPP