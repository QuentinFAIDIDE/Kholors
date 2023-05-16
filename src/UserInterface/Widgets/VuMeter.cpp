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
    
    int emptySidesWidth = (bounds.getWidth()-VUMETER_WIDTH)/2;
    bounds.reduce(emptySidesWidth, 0);

    // now our bounds perfectly fit the vumeter
    // area where we draw
    
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