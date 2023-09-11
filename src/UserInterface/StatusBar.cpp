#include "StatusBar.h"
#include "../Config.h"

void StatusBar::paint(juce::Graphics &g)
{
    auto bounds = g.getClipBounds();
    auto font = juce::Font(DEFAULT_FONT_SIZE);
    std::string versionPlaceholder = "Kholors Alpha v0.0.0";
    bounds.reduce(TAB_PADDING * 3, 0);
    g.setColour(COLOR_TEXT);
    g.setFont(font);
    g.drawText(versionPlaceholder, bounds, juce::Justification::centredLeft);
    g.drawText("Ready", bounds, juce::Justification::centredRight);
}