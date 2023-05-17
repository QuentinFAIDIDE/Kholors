#include "GeneratorsTab.h"

GeneratorsTab::GeneratorsTab()
{

}

GeneratorsTab::~GeneratorsTab()
{

}

void GeneratorsTab::paint(juce::Graphics&g)
{
    // we will display sections with their own subwidgets

    auto bounds = g.getClipBounds();

    juce::Colour bg = COLOR_BACKGROUND_LIGHTER;

    auto firstSectionArea = bounds.removeFromLeft(GENERATOR_TAB_SELECTOR_WIDTH);
    auto secondSectionArea = bounds;

    // first section is the selector
    drawSection(g, firstSectionArea, "Generator", bg);
    drawSection(g, secondSectionArea, "Generator", bg);
}