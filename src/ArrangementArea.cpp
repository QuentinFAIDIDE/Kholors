#include "ArrangementArea.h"

//==============================================================================
ArrangementArea::ArrangementArea()
{
    // save reference to the sample manager
    // initialize grid and position
}

ArrangementArea::~ArrangementArea()
{
    
}


//==============================================================================
void ArrangementArea::paint (juce::Graphics& g)
{

    juce::Rectangle<float> background(0.0, 0.0, 200.0, 120.0);
    g.fillCheckerBoard(background, 20, 10, juce::Colours::sandybrown, juce::Colours::saddlebrown);
    g.drawRect(background);

    // abort if nothing changed on the sample manager side

    // for each sample imported
        // get its position and size relative to view
        // filter it out if it's outside of view
        // get its freq-time image
        // add its pixels to the view buffer
        // add its intensity to the view buffer
}

void ArrangementArea::resized()
{
    // This is called when the MainComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
}