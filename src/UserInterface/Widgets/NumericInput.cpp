#include "NumericInput.h"
#include "../../Config.h"
#include <cstdio>
#include <string>

NumericInput::NumericInput(bool integers, float minValue, float maxValue, float stepValue, bool showButtons)
    : value(0.0f), isInteger(integers), min(minValue), max(maxValue)
{
}

void NumericInput::paint(juce::Graphics &g)
{
    g.setColour(COLOR_BACKGROUND.withAlpha(0.5f));
    g.fillRoundedRectangle(g.getClipBounds().toFloat(), 3);

    juce::Line<int> bottomLine(g.getClipBounds().getBottomLeft(), g.getClipBounds().getBottomRight());
    g.setColour(COLOR_TEXT_DARKER.withAlpha(0.15f));
    g.drawLine(bottomLine.toFloat(), 2.0f);

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
        snprintf(formatBuffer, FORMAT_BUFFER_MAXLEN, "%.2f\n", value);
        return std::string(formatBuffer);
    }
}