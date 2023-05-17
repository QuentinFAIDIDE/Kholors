#include "GeneratorsTab.h"

GeneratorsTab::GeneratorsTab()
{
}

GeneratorsTab::~GeneratorsTab()
{
}

void GeneratorsTab::paint(juce::Graphics &g)
{
    // we will display sections with their own subwidgets

    auto bounds = g.getClipBounds();
    bounds.reduce(TAB_PADDING, TAB_PADDING);

    juce::Colour bg = COLOR_BACKGROUND_LIGHTER;

    auto firstSectionArea =
        bounds.removeFromLeft(GENERATOR_TAB_SELECTOR_WIDTH).reduced(TAB_SECTIONS_MARGINS, TAB_SECTIONS_MARGINS);
    auto secondSectionArea = bounds.reduced(TAB_SECTIONS_MARGINS, TAB_SECTIONS_MARGINS);

    // first section is the selector
    drawSection(g, firstSectionArea, "Generator", bg);
    drawSection(g, secondSectionArea, "Generator", bg);
}