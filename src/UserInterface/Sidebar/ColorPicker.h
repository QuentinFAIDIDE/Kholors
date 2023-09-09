#ifndef DEF_COLOR_PICKER_HPP
#define DEF_COLOR_PICKER_HPP

#include "../../Arrangement/ActivityManager.h"
#include <juce_gui_basics/juce_gui_basics.h>

class ColorPicker : public juce::Component
{
  public:
    ColorPicker(ActivityManager &am);
    void paint(juce::Graphics &g) override;
    void mouseUp(const juce::MouseEvent &event) override;

  private:
    ActivityManager &activityManager;
    std::map<int, juce::Rectangle<int>> colorSquares;
};

#endif // DEF_COLOR_PICKER_HPP