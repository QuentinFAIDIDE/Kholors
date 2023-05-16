#include "VuMeter.h"
#include "../../Config.h"
#include "../Section.h"

VuMeter::VuMeter(std::string t, std::string id) : dbValueLeft(0.0), dbValueRight(0.0), title(t), identifier(id)
{
}

void VuMeter::paint(juce::Graphics &g)
{
    auto bounds = g.getClipBounds();

    // abort if no space available
    if (bounds.getWidth() < (VUMETER_WIDTH * 2))
    {
        return;
    }

    // draw the top title
    juce::Colour bg = juce::Colours::transparentBlack;
    drawSection(g, bounds, title, bg, true);

    auto boxesArea = zoomToInnerSection(bounds);

    // we then draw the core meter
    paintCoreMeter(g, boxesArea.withWidth(boxesArea.getWidth() / 2));
}

void VuMeter::mouseDrag(const juce::MouseEvent &event)
{
    // TODO
}

void VuMeter::setDbValue(float leftVal, float rightVal)
{
    dbValueLeft = leftVal;
    dbValueRight = rightVal;
}

juce::Rectangle<int> VuMeter::zoomToInnerSection(juce::Rectangle<int> bounds)
{
    // focus on the area without the title and margins
    auto boxesArea = bounds;
    boxesArea.removeFromTop(SECTION_TITLE_HEIGHT_SMALL);
    boxesArea.reduce(TOPBAR_WIDGETS_MARGINS, TOPBAR_WIDGETS_MARGINS);

    // what's the size of the remaining side parts ? (width x2 because grade is sise of vumeter)
    int emptySidesWidth = (boxesArea.getWidth() - (VUMETER_WIDTH * 2)) / 2;

    // remove the side areas
    boxesArea.reduce(emptySidesWidth, 0);
    return boxesArea;
}

void VuMeter::paintCoreMeter(juce::Graphics &g, juce::Rectangle<int> boxesArea)
{
    // we then draw the inside of the vumeter
    g.setColour(COLOR_BACKGROUND);
    g.fillRect(boxesArea);
    g.setColour(COLOR_TEXT_DARKER.withAlpha(0.5f));
    g.drawRect(boxesArea);
}