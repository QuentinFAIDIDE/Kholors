#include "Section.h"

#include "../Config.h"
#include "FontsLoader.h"

void drawSection(juce::Graphics &g, juce::Rectangle<int> &bounds, juce::String title)
{
    g.setColour(COLOR_TEXT_DARKER);

    juce::SharedResourcePointer<FontsLoader> sharedFonts;
    juce::Font robotBold(sharedFonts->robotoBold);
    robotBold.setHeight(SMALLER_FONT_SIZE);
    g.setFont(robotBold);

    int titleHeight = SECTION_TITLE_HEIGHT;

    g.drawText(title, bounds.getX(), bounds.getY(), bounds.getWidth(), titleHeight, juce::Justification::centredLeft,
               false);
}