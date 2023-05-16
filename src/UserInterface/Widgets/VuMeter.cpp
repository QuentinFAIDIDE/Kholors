#include "VuMeter.h"
#include "../Section.h"

VuMeter::VuMeter(std::string t) title(t)
{

}

void VuMeter::paint(juce::Graphics &g)
{
    auto bounds = g.getClipBounds();
    
    if (bounds.getWidth() < (VUMETER_WIDTH+(2*margins)) )
    {
        return;
    }

    juce::Colour bg = juce::Colours::transparentBlack;
    drawSection(g, bounds, title, bg);

    // get widget area
    auto boxesArea = bounds;
    boxesArea.removeFromTop(SECTION_TITLE_HEIGHT);
    boxesArea.reduce(margins, margins);
    
    // TODO: remove two text sections from top to store
    // some sort of averaged last max value (that get resets when clicked ?)

    int emptySidesWidth = (bounds.getWidth()-VUMETER_WIDTH)/2;
    // TODO: draw scales in those empty area
    bounds.reduce(emptySidesWidth, 0);

    // now our bounds perfectly fit the vumeter
    // area where we draw
    g.setColor(COLOR_TEXT_DARKER);
    g.drawRect(bounds);
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