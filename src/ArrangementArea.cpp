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

    // get the window width
    juce::Rectangle<int> bounds = g.getClipBounds();

    // draw nothing if the windows is too small
    if(bounds.getWidth()<MIN_SCREEN_WIDTH || bounds.getHeight()<MIN_SCREEN_HEIGHT) {
        g.fillAll(juce::Colour(20, 20, 20));
        return;
    }

    // set the arrangement area size to 600 pixels at the middle of the screen
    juce::Rectangle<int> background(
        0,
        (bounds.getHeight() - FREQTIME_VIEW_HEIGHT)>>1,
        bounds.getWidth(),
        FREQTIME_VIEW_HEIGHT
    );
    g.setColour(juce::Colour(20, 20, 20));
    g.fillRect(background);

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