#include "Section.h"

#include "../Config.h"

void drawSection(juce::Graphics &g, juce::Rectangle<int> &bounds, juce::String title, juce::Colour &background)
{
    g.setColour(background);
    g.fillRoundedRectangle(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), 4);

    g.setColour(COLOR_TEXT_DARKER);
    g.drawRoundedRectangle(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), 4, 0.4f);

    g.setColour(COLOR_TEXT_DARKER);
    g.setFont(juce::Font(DEFAULT_FONT_SIZE));
    g.drawText(title, bounds.getX(), bounds.getY() + 3, bounds.getWidth(), 18, juce::Justification::centred, false);
}