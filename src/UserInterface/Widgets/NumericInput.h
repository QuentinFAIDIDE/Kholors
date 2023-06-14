#ifndef DEF_NUMERIC_INPUT_HPP
#define DEF_NUMERIC_INPUT_HPP

#include "../CustomFonts.h"

#include <juce_gui_basics/juce_gui_basics.h>

#define NUMERIC_INPUT_TEXT_MARGINS 3
#define NUMERIC_INPUT_SIDE_ADDITIONAL_MARGINS 8

class NumericInput : public juce::Component
{
  public:
    NumericInput(bool integers, float minValue, float maxValue, float stepValue, bool showButtons);
    void paint(juce::Graphics &g) override;
    void setValue(float val);

  private:
    // the actual displayed value
    float value;
    // are we using integers or float ?
    bool isInteger;
    // minimum value the input can take
    float min;
    // maximum value the input can take
    float max;
    juce::SharedResourcePointer<CustomFonts> sharedFonts;

    /**
     * Get the string from the value depending
     * on if integer or not
     */
    std::string getStringValue();
};

#endif // DEF_NUMERIC_INPUT_HPP