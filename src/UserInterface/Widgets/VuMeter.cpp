#include "VuMeter.h"
#include "../../Config.h"
#include "../Section.h"

VuMeter::VuMeter(std::string t) : title(t), dbValueLeft(0.0), dbValueRight(0.0), dbMaxLeft(0.0), dbMaxRight(0.0)
{
}

void VuMeter::paint(juce::Graphics &g)
{
    auto bounds = g.getClipBounds();

    // abort if no space available
    if (bounds.getWidth() < (VUMETER_WIDTH + (2 * TOPBAR_SECTIONS_INNER_MARGINS)))
    {
        return;
    }

    // draw the top title
    juce::Colour bg = juce::Colours::transparentBlack;
    drawSection(g, bounds, title, bg);

    auto boxesArea = zoomToInnerSection(bounds);

    paintBottomValues(g, boxesArea);

    // we then draw the core meter
    paintCoreMeter(g, boxesArea);
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
    boxesArea.removeFromTop(SECTION_TITLE_HEIGHT);
    boxesArea.reduce(TOPBAR_SECTIONS_INNER_MARGINS, TOPBAR_SECTIONS_INNER_MARGINS);

    // what's the size of the remaining side parts ?
    int emptySidesWidth = (boxesArea.getWidth() - VUMETER_WIDTH) / 2;
    // TODO: draw scales in those empty area

    // remove the side areas
    boxesArea.reduce(emptySidesWidth, 0);
    return boxesArea;
}

void VuMeter::paintCoreMeter(juce::Graphics &g, juce::Rectangle<int> boxesArea)
{
    // TODO
}

void VuMeter::paintBottomValues(juce::Graphics &g, juce::Rectangle<int> boxesArea)
{
    // extract the bottom area where moving average values are show
    auto maxValArea = boxesArea.removeFromBottom(VUMETER_MAXVAL_HEIGHT);

    // isolate left and right text area
    g.setColour(COLOR_TEXT_DARKER);
    auto leftTxtVal = maxValArea.withWidth(VUMETER_WIDTH / 2);
    auto rightTxtVal = leftTxtVal.withY(leftTxtVal.getY() + (VUMETER_WIDTH / 2));

    // draw the text
    g.drawText(juce::String(dbMaxLeft), leftTxtVal, juce::Justification::centred, true);
    g.drawText(juce::String(dbMaxRight), rightTxtVal, juce::Justification::centred, true);

    // we then draw the inside of the vumeter
    g.setColour(COLOR_BACKGROUND);
    g.fillRect(boxesArea);
    g.setColour(COLOR_TEXT_DARKER);
    g.drawRect(boxesArea);
}