#include "ArrangementArea.h"

#include <cmath>
#include <iostream>

//======== COMMENT =========
// We often mention "audio frames" in the code.
// Standard name is "audio samples" but this can be
// confused with the audio samples we are supposed to edit
// with this software.
// So when

//==============================================================================
ArrangementArea::ArrangementArea(SampleManager& sm, NotificationArea& na) :
  sampleManager(sm),
  notificationArea(na) 
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
  // bars drawned in order, watch for overlaps (use smaller subdiv first)
  gridSubdivisions.push_back((GridLevel){4.0, 80});
  gridSubdivisions.push_back((GridLevel){1.0, 160});
  gridSubdivisions.push_back((GridLevel){0.25, 200});
}

ArrangementArea::~ArrangementArea() {}

//==============================================================================
void ArrangementArea::paint(juce::Graphics& g) {
  // get the window width
  bounds = g.getClipBounds();

  // draw nothing if the windows is too small
  if (bounds.getWidth() < MIN_SCREEN_WIDTH ||
      bounds.getHeight() < MIN_SCREEN_HEIGHT) {
    g.fillAll(juce::Colour(20, 20, 20));
    return;
  }

  // does the grid needs to be recomputed ?
  // if so, can we shift it to save processing ?
  // OPTIMIZATION: can we maybe transform the grid to save some pixel if resized
  // ? update the grid at required pixels

  // draw the background and grid
  paintBars(g);

  // draw samples
}

void ArrangementArea::resized() {
  // This is called when the MainComponent is resized.
  // If you add any child components, this is where you should
  // update their positions.
}

// warning buggy with bars over 4 subdivisions due to << operator
void ArrangementArea::paintBars(juce::Graphics& g) {
  // TODO: make this function work for subdivision above power of 3

  // set the arrangement area size to 600 pixels at the middle of the screen
  juce::Rectangle<int> background(
      0, (bounds.getHeight() - FREQTIME_VIEW_HEIGHT) >> 1, bounds.getWidth(),
      FREQTIME_VIEW_HEIGHT);
  // paint the background of the area
  g.setColour(juce::Colour(20, 20, 20));
  g.fillRect(background);

  // width of the bar grid
  int barGridPixelWidth =
      floor((float(AUDIO_FRAMERATE * 60) / float(tempo)) / float(viewScale)) +
      1;
  // two buffer values for the next for loop
  int64_t subdivisionWidth, subdivisionShift;

  // for each subdivision in the reversed order
  for (int i = 0; i < gridSubdivisions.size(); i++) {
    // subdivided bar length
    subdivisionWidth = barGridPixelWidth / gridSubdivisions[i].subdivisions;

    // pixel shifting required by view position.
    subdivisionShift =
        (barGridPixelWidth - ((viewPosition / viewScale) % barGridPixelWidth));

    if (subdivisionWidth > 25) {
      // set the bar color
      g.setColour(juce::Colour((int)gridSubdivisions[i].shade,
                               (int)gridSubdivisions[i].shade,
                               (int)gridSubdivisions[i].shade));

      // we won't use subdivisionWidth to add it as it has a small
      // error that will accumulate over bars and create
      // displacements.

      // subdivision ratio
      float subdivisionRatio = 1.0 / gridSubdivisions[i].subdivisions;

      // iterate while it's possible to draw the successive bars
      int barPositionX = 0;
      for (int j = -gridSubdivisions[i].subdivisions;
           barPositionX < bounds.getWidth(); j++) {
        barPositionX = subdivisionShift +
                       (j * barGridPixelWidth * subdivisionRatio) -
                       barGridPixelWidth;
        // draw the bar
        g.drawLine(barPositionX, background.getY(), barPositionX,
                   background.getY() + FREQTIME_VIEW_HEIGHT);
      }
    }
  }
}

void ArrangementArea::mouseDown(const juce::MouseEvent& jme) {
  // if the mouse was clicked
  if (jme.mouseWasClicked()) {
    // if the button click was a middle mouse
    if (jme.mods.isMiddleButtonDown()) {
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
  if (isResizing) {
    // TODO: save some repait by comparing viewScale
    // TODO: smarter movement with time implied ?

    // ratio from horizontal to vertical movement
    float movementRatio = ((float)abs(lastMouseX - newPosition.getX())) /
                          ((float)abs(lastMouseY - newPosition.getY()));

    // if ratio from horizontal to vertical is higher than thresold
    if (movementRatio > FREQVIEW_MIN_HORIZONTAL_MOVE_RATIO) {
      // move for horizontal position
      pixelMovement = newPosition.getX() - lastMouseX;
      frameMovement = pixelMovement * viewScale;
      // avoid having the position going below zero
      if (frameMovement > 0 && frameMovement <= viewPosition) {
        viewPosition -= frameMovement;
      } else if (frameMovement < 0) {
        viewPosition -= frameMovement;
      }
    }

    // if ratio from horizontal to vertical movement is smaller than half
    if (movementRatio < FREQVIEW_MIN_VERTICAL_MOVE_RATIO) {
      // scale for vertical movements
      pixelMovement = newPosition.getY() - lastMouseY;
      // cap the pixel movement to avoid harsh movements
      if (abs(pixelMovement) > FREQVIEW_MAX_ABSOLUTE_SCALE_MOVEMENT) {
        // this weird instruction is getting the sign of the integer fast
        pixelMovement = ((pixelMovement > 0) - (pixelMovement < 0)) *
                        FREQVIEW_MAX_ABSOLUTE_SCALE_MOVEMENT;
      }
      viewScale = viewScale + pixelMovement;
      // check if within bounds
      if (viewScale < FREQVIEW_MIN_SCALE_FRAME_PER_PIXEL) {
        viewScale = FREQVIEW_MIN_SCALE_FRAME_PER_PIXEL;
      }
      if (viewScale > FREQVIEW_MAX_SCALE_FRAME_PER_PIXEL) {
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
  if (isResizing && !jme.mods.isMiddleButtonDown()) {
    // save that we are not in resize mode anymore
    isResizing = false;
  }

  // saving last mouse position
  juce::Point<int> newPosition = jme.getPosition();
  lastMouseX = newPosition.getX();
  lastMouseY = newPosition.getY();
}

bool ArrangementArea::isInterestedInFileDrag(const juce::StringArray& files) {
  // are the files sound files that are supported
  return sampleManager.filePathsValid(files);
}

void ArrangementArea::filesDropped(const juce::StringArray& files, int x,
                                   int y) {
    // to know if one sample was imported and we need to update
    bool refreshNeeded = false;
    // converts x to an valid position in audio frame
    int64_t framePos = viewPosition + (x*viewScale);
    // we try to load the samples
    for(int i=0; i<files.size(); i++) {
        int id = sampleManager.addSample(files[i], framePos);
        // on failures, abort
        if(id == -1) {
            // notify error
            notificationArea.notifyError(files[i]);
            continue;
        }
    }
    if(refreshNeeded) {
        repaint();
    }
}