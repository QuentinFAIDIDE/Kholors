#include "TimeInfo.h"

#include "../../Config.h"

TimeInfo::TimeInfo() : frameValue(0)
{
}

void TimeInfo::paint(juce::Graphics &g)
{
    g.setColour(COLOR_BACKGROUND.withAlpha(0.5f));
    g.fillRoundedRectangle(g.getClipBounds().toFloat(), 3);

    juce::Line<int> bottomLine(g.getClipBounds().getBottomLeft(), g.getClipBounds().getBottomRight());
    g.setColour(COLOR_TEXT_DARKER.withAlpha(0.15f));
    g.drawLine(bottomLine.toFloat(), 2.0f);

    auto textArea = g.getClipBounds().reduced(TIMEINFO_TEXT_MARGINS, TIMEINFO_TEXT_MARGINS);
    g.setColour(COLOR_TEXT_DARKER);
    g.setFont(sharedFonts->monospaceFont.withHeight(SMALLER_FONT_SIZE));
    g.drawText("00m 00s 000ms", textArea, juce::Justification::centred);
}