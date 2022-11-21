#include "ArrangementArea.h"
#include <iostream>

//==============================================================================
ArrangementArea::ArrangementArea()
{
    // save reference to the sample manager
    // initialize grid and position
    viewPosition = 0;
    viewScale = 100;
    lastMouseX = 0;
    lastMouseY = 0;
    // TODO: configure this in args or config file
    tempo = 120;
    gridSubdivisions.push_back((GridLevel){1, 160});
}

ArrangementArea::~ArrangementArea()
{
    
}


//==============================================================================
void ArrangementArea::paint (juce::Graphics& g)
{

    // get the window width
    bounds = g.getClipBounds();

    // draw nothing if the windows is too small
    if(bounds.getWidth()<MIN_SCREEN_WIDTH || bounds.getHeight()<MIN_SCREEN_HEIGHT) {
        g.fillAll(juce::Colour(20, 20, 20));
        return;
    }

    // does the grid needs to be recomputed ? 
        // if so, can we shift it to save processing ?
        // OPTIMIZATION: can we maybe transform the grid to save some pixel if resized ?
        // update the grid at required pixels

    // draw the background and grid
    paintBars(g);

    // draw samples
}

void ArrangementArea::resized()
{
    // This is called when the MainComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
}

void ArrangementArea::paintBars(juce::Graphics& g) {

    // set the arrangement area size to 600 pixels at the middle of the screen
    juce::Rectangle<int> background(
        0,
        (bounds.getHeight() - FREQTIME_VIEW_HEIGHT)>>1,
        bounds.getWidth(),
        FREQTIME_VIEW_HEIGHT
    );
    // paint the background of the area
    g.setColour(juce::Colour(20, 20, 20));
    g.fillRect(background);

    // width of the bar grid
    int barGridPixelWidth = ((AUDIO_FRAMERATE*60) / (tempo*viewScale));
    // for each subdivision in the reversed order
    for(int i=gridSubdivisions.size()-1; i>=0; i--) {
        // set the bar color
        g.setColour(juce::Colour(
            (int)gridSubdivisions[i].shade,
            (int)gridSubdivisions[i].shade,
            (int)gridSubdivisions[i].shade));
        // iterate while it's possible to draw the successive bars
        for(
            int64_t barPositionX=0;
            barPositionX<=(int64_t)bounds.getWidth();
            barPositionX+= (int64_t)(barGridPixelWidth>>(gridSubdivisions[i].subdivision-1))
        ) {
            // draw the bar
            g.drawLine(
                barPositionX,
                background.getY(),
                barPositionX,
                background.getY() + FREQTIME_VIEW_HEIGHT
            );
        }
    }
}

void ArrangementArea::mouseDown(const juce::MouseEvent&) {

}

void ArrangementArea::mouseUp(const juce::MouseEvent&) {

}

void ArrangementArea::mouseMove(const juce::MouseEvent&) {

}