#include "Section.h"

#include "../Config.h"

void drawSection(juce::Graphics &g, juce::Rectangle<int> &bounds, juce::String title, juce::Colour &background,
                 bool small)
{
    g.setColour(background);
    g.fillRoundedRectangle(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), 4);

    if (!small)
    {
        g.setColour(COLOR_TEXT_DARKER);
        g.drawRoundedRectangle(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), 4, 0.4f);
    }

    g.setColour(COLOR_TEXT_DARKER);
    g.setFont(juce::Font(DEFAULT_FONT_SIZE));

    int titleHeight = SECTION_TITLE_HEIGHT;

    // large section could use top padding to be centered
    int topPadding = 2;

    if (small)
    {
        g.setFont(juce::Font(DEFAULT_FONT_SIZE * 0.8));
        titleHeight = SECTION_TITLE_HEIGHT_SMALL;

        // small section could use less top padding
        topPadding = -2;
    }

    g.drawText(title, bounds.getX(), bounds.getY() + topPadding, bounds.getWidth(), titleHeight,
               juce::Justification::centred, false);
}