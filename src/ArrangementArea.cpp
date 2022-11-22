#include "ArrangementArea.h"
#include <iostream>
#include <cmath>

//======== COMMENT =========
// We often mention "audio frames" in the code.
// Standard name is "audio samples" but this can be
// confused with the audio samples we are supposed to edit
// with this software.
// So when 

//==============================================================================
ArrangementArea::ArrangementArea()
{
    // save reference to the sample manager
    // initialize grid and position
    viewPosition = 0;
    viewScale = 100;
    lastMouseX = 0;
    lastMouseY = 0;
    isResizing = false;
    // TODO: configure this in args or config file
    tempo = 120;
    // WARNING: they will be drawned reversed order
    // so watch out 
    gridSubdivisions.push_back((GridLevel){1, 160});
    gridSubdivisions.push_back((GridLevel){3, 80});
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

// warning buggy with bars over 4 subdivisions due to << operator
void ArrangementArea::paintBars(juce::Graphics& g) {

    // TODO: make this function work for subdivision above power of 3

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
    // two buffer values for the next for loop
    int64_t subdivisionWidth, subdivisionShift;

    // for each subdivision in the reversed order
    for(int i=gridSubdivisions.size()-1; i>=0; i--) {

        // subdivided bar length
        subdivisionWidth = barGridPixelWidth>>(gridSubdivisions[i].subdivision-1);

        // pixel shifting required by view position.
        // its the distance to the previous bar converted from audio frames to pixels
        subdivisionShift = subdivisionWidth-((viewPosition/viewScale) % subdivisionWidth);

        if (subdivisionWidth > 20) {
            // set the bar color
            g.setColour(juce::Colour(
                (int)gridSubdivisions[i].shade,
                (int)gridSubdivisions[i].shade,
                (int)gridSubdivisions[i].shade));
            // iterate while it's possible to draw the successive bars
            for(
                int64_t barPositionX=subdivisionShift;
                barPositionX<=(int64_t)bounds.getWidth();
                barPositionX+= subdivisionWidth
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
}

void ArrangementArea::mouseDown(const juce::MouseEvent& jme) {
    // if the mouse was clicked
    if(jme.mouseWasClicked()) {
        // if the button click was a middle mouse
        if(jme.mods.isMiddleButtonDown()) {
            // save that we are in resize mode
            isResizing = true;
        }
    }
    // saving last mouse position
    juce::Point<int> position = jme.getPosition();
    lastMouseX = position.getX();
    lastMouseY = position.getY();
}

void ArrangementArea::mouseUp(const juce::MouseEvent& jme) {

    // saving last mouse position
    juce::Point<int> position = jme.getPosition();
    lastMouseX = position.getX();
    lastMouseY = position.getY();
}

void ArrangementArea::mouseDrag(const juce::MouseEvent& jme) {

    juce::Point<int> newPosition = jme.getPosition();

    // some preallocated values
    int pixelMovement, frameMovement;

    // if in resize mode
    if(isResizing) {
        // TODO: save some repait by comparing viewScale
        // TODO: smarter movement with time implied ?

        // ratio from horizontal to vertical movement
        float movementRatio = 
            ( (float)abs(lastMouseX - newPosition.getX()) )
            /
            ( (float)abs(lastMouseY - newPosition.getY()) );


        // if ratio from horizontal to vertical is higher than thresold
        if (movementRatio > FREQVIEW_MIN_HORIZONTAL_MOVE_RATIO) {
            // move for horizontal position
            pixelMovement = newPosition.getX() - lastMouseX;
            frameMovement = pixelMovement * viewScale;
            // avoid having the position going below zero
            if(frameMovement > 0 && frameMovement <= viewPosition) {
                viewPosition -= frameMovement;
            } else if(frameMovement < 0) {
                viewPosition -= frameMovement;
            }
        }

        // if ratio from horizontal to vertical movement is smaller than half
        if (movementRatio < FREQVIEW_MIN_VERTICAL_MOVE_RATIO) {
            // scale for vertical movements
            pixelMovement = newPosition.getY() - lastMouseY;
            viewScale = viewScale + pixelMovement;
            // check if within bounds
            if(viewScale < FREQVIEW_MIN_SCALE_FRAME_PER_PIXEL) {
                viewScale = FREQVIEW_MIN_SCALE_FRAME_PER_PIXEL;
            }
            if(viewScale > FREQVIEW_MAX_SCALE_FRAME_PER_PIXEL) {
                viewScale = FREQVIEW_MAX_SCALE_FRAME_PER_PIXEL;
            }
        }
        repaint();
    }

    // saving last mouse position
    lastMouseX = newPosition.getX();
    lastMouseY = newPosition.getY();
}


void ArrangementArea::mouseMove(const juce::MouseEvent& jme) {

    // if we are in resize mode and middle mouse button is pressed
    if(isResizing && !jme.mods.isMiddleButtonDown()) {
        // save that we are not in resize mode anymore
        isResizing = false;
    }

    // saving last mouse position
    juce::Point<int> newPosition = jme.getPosition();
    lastMouseX = newPosition.getX();
    lastMouseY = newPosition.getY();
}