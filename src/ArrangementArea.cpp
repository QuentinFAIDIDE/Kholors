#include "ArrangementArea.h"

//==============================================================================
ArrangementArea::ArrangementArea()
{
    // save reference to the sample manager
    // initialize grid and position
    viewPosition = 0;
    viewScale = 100;
    // TODO: configure this in args
    tempo = 120;
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
    paintGrid(g);

    // draw samples
}

void ArrangementArea::resized()
{
    // This is called when the MainComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
}

void ArrangementArea::paintGrid(juce::Graphics& g) {
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
    int barGridFrameWidth = (AUDIO_FRAMERATE*60) / tempo;
    // color of the bar grid
    g.setColour(juce::Colour(200, 200, 200));
    // draw the tempo grid as long as there are within borders
    for (int posBufferX = 0; posBufferX <= background.getWidth(); posBufferX += barGridFrameWidth) {
        g.drawLine(
            posBufferX,
            background.getY(),
            posBufferX,
            background.getY() + FREQTIME_VIEW_HEIGHT
        );
    }
    // color of the quarter bar grid
    g.setColour(juce::Colour(150, 150, 150));
    // draw bar quarters (TODO: make a function to do subdivions)
    for (int posBufferX = 0; posBufferX <= background.getWidth(); posBufferX += barGridFrameWidth>>2) {
        if (posBufferX % barGridFrameWidth == 0) {
            // skip lines that overlap full bars
            continue;
        }
        g.drawLine(
            posBufferX,
            background.getY(),
            posBufferX,
            background.getY() + FREQTIME_VIEW_HEIGHT
        );
    }
}