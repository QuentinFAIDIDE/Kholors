#ifndef DEF_NUMERIC_INPUT_HPP
#define DEF_NUMERIC_INPUT_HPP

#include "../../Arrangement/ActivityManager.h"
#include "../CustomFonts.h"

#include <juce_gui_basics/juce_gui_basics.h>

#define NUMERIC_INPUT_TEXT_MARGINS 3
#define NUMERIC_INPUT_SIDE_ADDITIONAL_MARGINS 8

#define FORMAT_BUFFER_MAXLEN 128

class NumericInput : public juce::Component
{
  public:
    NumericInput(bool integers, float minValue, float maxValue, float stepValue);
    void paint(juce::Graphics &g) override;
    void setValue(float val);
    void setActivity(ActivityManager *am, int inputId);

  private:
    // the actual displayed value
    float value;
    // are we using integers or float ?
    bool isInteger;
    // minimum value the input can take
    float min;
    // maximum value the input can take
    float max;
    // what's the lower unit of modification
    float step;

    // the shared fonts
    juce::SharedResourcePointer<CustomFonts> sharedFonts;

    // the identifier of the numeric input to pop appropriate update tasks
    int numericInputId;

    /**
     * Get the string from the value depending
     * on if integer or not
     */
    std::string getStringValue();

    char formatBuffer[FORMAT_BUFFER_MAXLEN];
};

#endif // DEF_NUMERIC_INPUT_HPP