#ifndef DEF_STATUS_BAR_HPP
#define DEF_STATUS_BAR_HPP

#include "StatusTips.h"
#include "juce_gui_basics/juce_gui_basics.h"

#define VERSION_PLACEHOLDER_WIDTH 220
#define ACTION_PLACEHOLDER_WIDTH 350

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