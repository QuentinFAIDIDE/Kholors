#include "NumericInput.h"
#include "../../Config.h"
#include <format>
#include <string>

NumericInput::NumericInput(bool integers, float minValue, float maxValue, float stepValue, bool showButtons)
{
}

void NumericInput::paint(juce::Graphics &g)
{
    g.setColour(COLOR_BACKGROUND.withAlpha(0.5f));
    g.fillRoundedRectangle(g.getClipBounds().toFloat(), 3);

    auto textArea = g.getClipBounds().reduced(NUMERIC_INPUT_TEXT_MARGINS + NUMERIC_INPUT_SIDE_ADDITIONAL_MARGINS,
                                              NUMERIC_INPUT_TEXT_MARGINS);

    g.setColour(COLOR_TEXT_DARKER);
    g.setFont(sharedFonts->monospaceFont.withHeight(SMALLER_FONT_SIZE));
    g.drawText(getStringValue(), textArea, juce::Justification::centredRight);
}

void NumericInput::setValue(float val)
{
    value = val;
}

std::string NumericInput::getStringValue()
{
    if (isInteger)
    {
        return std::to_string(int(value + 0.5f));
    }
    else
    {
        return std::format("{:.2f}", value);
    }
}