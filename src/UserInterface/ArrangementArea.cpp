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
ArrangementArea::ArrangementArea(SampleManager& sm, NotificationArea& na)
    : sampleManager(sm),
      notificationArea(na) {
  // save reference to the sample manager
  // initialize grid and position
  viewPosition = 0;
  viewScale = 100;
  lastMouseX = 0;
  lastMouseY = 0;
  isResizing = false;
  isMovingCursor = false;
  lastPlayCursorPosition = 0;
  trackMovingInitialPosition = -1;
  // TODO: make fft block height dynamic for zooming on y axis
  fftBlockHeight = float(FREQTIME_VIEW_HEIGHT-FREQTIME_VIEW_INNER_MARGINS)/float(FREQVIEW_SAMPLE_FFT_SIZE<<1);
  // TODO: configure this in args or config file
  tempo = 120;
  // bars drawned in order, watch for overlaps (use smaller subdiv first)
  gridSubdivisions.push_back((GridLevel){4.0, 80});
  gridSubdivisions.push_back((GridLevel){1.0, 160});
  gridSubdivisions.push_back((GridLevel){0.25, 200});
  // play cursor color
  cursorColor = juce::Colour(240, 240, 240);
  // enable keyboard events
  setWantsKeyboardFocus(true);
}

ArrangementArea::~ArrangementArea() {}

void ArrangementArea::paint(juce::Graphics& g) {
  // get the window width
  bounds = g.getClipBounds();

  // draw nothing if the windows is too small
  if (bounds.getWidth() < FREQVIEW_MIN_WIDTH ||
      bounds.getHeight() < FREQVIEW_MIN_HEIGHT) {
    g.fillAll(juce::Colour(20, 20, 20));
    return;
  }

  // draw the background and grid
  paintBars(g);

  // draw samples
  paintSamples(g);

  // paint the play cursor
  paintPlayCursor(g);
}

void ArrangementArea::resized() {
  // This is called when the MainComponent is resized.
  // If you add any child components, this is where you should
  // update their positions.
}

// warning buggy with bars over 4 subdivisions due to << operator
void ArrangementArea::paintBars(juce::Graphics& g) {
  // TODO: this algorithm is highly sub optimal and accumulates small errors in
  // grid positions. It needs to be reimplemented.

  // set the arrangement area size to 600 pixels at the middle of the screen
  juce::Rectangle<int> background(0, 0, bounds.getWidth(),
                                  FREQTIME_VIEW_HEIGHT);
  // paint the background of the area
  g.setColour(juce::Colour(20, 20, 20));
  g.fillRect(background);

  // width of the bar grid
  int barGridPixelWidth =
      floor((float(AUDIO_FRAMERATE * 60) / float(tempo)) / float(viewScale));
  // two buffer values for the next for loop
  float subdivisionWidth, subdivisionShift, subdivisionWidthFrames;

  // for each subdivision in the reversed order
  for (int i = 0; i < gridSubdivisions.size(); i++) {
    // subdivided bar length (0.5f addedd to round instead of truncate)
    subdivisionWidth =
        ((float(barGridPixelWidth) / gridSubdivisions[i].subdivisions));
    subdivisionWidthFrames = subdivisionWidth * float(viewScale);

    // pixel shifting required by view position.
    subdivisionShift = (subdivisionWidthFrames -
                        float(viewPosition % int(subdivisionWidthFrames))) /
                       float(viewScale);

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
      for (int j = 0; barPositionX < bounds.getWidth(); j++) {
        barPositionX = int(0.5 + subdivisionShift +
                           (j * barGridPixelWidth * subdivisionRatio));
        // draw the bar
        g.drawLine(barPositionX, 0, barPositionX, FREQTIME_VIEW_HEIGHT);
      }
    }
  }
}

void ArrangementArea::paintSamples(juce::Graphics& g) {
  // NOTE: We never delete SamplePlayers in the tracks array or insert mid-array
  // to prevent using a lock when reading.

  size_t nTracks = sampleManager.getNumTracks();

  // reference to the samplePlayer we are drawing in the loop
  SamplePlayer* sp;

  // buffer values for each track
  int64_t trackLeftBound;
  int64_t trackRightBound;

  // the leftmost on-screen position in audio samples
  int64_t viewRightBound = viewPosition + (viewScale * bounds.getWidth());

  noVerticalSquaresFft = (FREQTIME_VIEW_HEIGHT-FREQTIME_VIEW_INNER_MARGINS)/(FREQVIEW_SAMPLE_FFT_RESOLUTION_PIXELS*2);

  // for each track
  for (size_t i = 0; i < nTracks; i++) {
    // get a reference to the sample
    sp = sampleManager.getTrack(i);
    // skip nullptr in tracks list (should not happen)
    if (sp == nullptr) {
      continue;
    }
    // get its lock
    const juce::SpinLock::ScopedLockType lock(sp->playerMutex);
    // compute left and right bounds
    trackLeftBound = sp->getEditingPosition();
    trackRightBound = trackRightBound + sp->getLength();
    // if it's in bound
    if (trackRightBound > viewPosition && trackLeftBound < viewRightBound) {
      // draw it
      drawSampleTrack(g, sp, i);
    }
  }
}

void ArrangementArea::drawSampleTrack(juce::Graphics& g, SamplePlayer* sp, size_t id) {
  // the shift to apply on tracks currently dragged
  int64_t dragShift = 0;
  auto search = selectedTracks.find(id);
  if(trackMovingInitialPosition!=-1 && search != selectedTracks.end()) {
    dragShift = (lastMouseX - trackMovingInitialPosition);
  }
  float radius;
  // if the track is currently being selected, draw thicker borders
  if(search != selectedTracks.end()) {
    radius = SAMPLEPLAYER_BORDER_WIDTH;
  } else {
    radius = SAMPLEPLAYER_BORDER_WIDTH/2;
  }
  // draw a rectangle around the sample
  g.setColour(SAMPLEPLAYER_BORDER_COLOR);
  auto positionX = ((sp->getEditingPosition() - viewPosition) / viewScale)+dragShift;
  auto positionY = FREQTIME_VIEW_INNER_MARGINS>>1;
  g.drawRoundedRectangle(positionX, positionY, sp->getLength() / viewScale,
                         FREQTIME_VIEW_HEIGHT-(FREQTIME_VIEW_INNER_MARGINS),
                         SAMPLEPLAYER_BORDER_RADIUS, radius);
  // if there are two channels
  if(sp->getBufferNumChannels()==2) {
    // draw left (upper on the chart) channel
    drawSampleChannelFft(g, sp, positionX, FREQTIME_VIEW_INNER_MARGINS>>1, 0, false);
    // draw right (lower on the chart) channel
    drawSampleChannelFft(g, sp, positionX, FREQTIME_VIEW_HEIGHT>>1, 1, true);
  // if buffer has anything other than two channels only display first
  } else {
    // duplicate the first channel (most likely it's mono)
    drawSampleChannelFft(g, sp, positionX, FREQTIME_VIEW_INNER_MARGINS>>1, 0, false);
    drawSampleChannelFft(g, sp, positionX, FREQTIME_VIEW_HEIGHT>>1, 0, true);
  }
}

// drawSampleChannelFft draw the result of the fft of the sample for this channel
// at this position. It will display lower freq to bottom if flipped is false.
void ArrangementArea::drawSampleChannelFft(juce::Graphics& g, SamplePlayer *sp,
  int64_t positionX, int64_t positionY, int channel, bool flipped) {

  float intensity;

  int noHorizontalSquares = int((sp->getLength() / viewScale) / (FREQVIEW_SAMPLE_FFT_RESOLUTION_PIXELS));

  int posX, posY;

  // TODO: precast values like float(FREQVIEW_SAMPLE_FFT_SIZE) and float(noHorizontalSquares)

  // for each horizontal line
  for(size_t i=0; i<noHorizontalSquares; i++) {
    // compute index of the fft to read
    posX = int((float(i)/float(noHorizontalSquares))*float(sp->getNumFft()));
    // for each vertical line
    for(size_t j=0; j<noVerticalSquaresFft; j++) {
      if(flipped) {
        posY = int(std::round(
          polylens((float(j))/float(noVerticalSquaresFft))*
          float(FREQVIEW_SAMPLE_FFT_SCOPE_SIZE)
        ));
      } else {
        posY = int(std::round(
          polylens(1.0f-((float(j))/float(noVerticalSquaresFft)))*
          float(FREQVIEW_SAMPLE_FFT_SCOPE_SIZE)
        ));
      }
      // get the intensity at this frequency/time
      intensity = sp->getFftData()[
        
        (channel * sp->getNumFft() * FREQVIEW_SAMPLE_FFT_SCOPE_SIZE) +
        (posX*FREQVIEW_SAMPLE_FFT_SCOPE_SIZE) + posY

      ];
      // scale intensity to fit between 0 and 1
      intensity = juce::jmap(intensity, MIN_DB, MAX_DB, 0.0f, 1.0f);
      intensity = sigmoid((intensity*12.0)-6.0);
      intensity = juce::jlimit(0.0f, 1.0f, intensity);
      // draw the rectangle
      g.setColour(sp->getColor().withAlpha(intensity));
      g.fillRect(
        positionX + FREQVIEW_SAMPLE_FFT_RESOLUTION_PIXELS*i, // TODO: precompute out of loop
        positionY + FREQVIEW_SAMPLE_FFT_RESOLUTION_PIXELS*j,
        FREQVIEW_SAMPLE_FFT_RESOLUTION_PIXELS,
        FREQVIEW_SAMPLE_FFT_RESOLUTION_PIXELS
      );
    }
  }
}

float ArrangementArea::polylens(float v) {
  if(v<0.5) {
    return std::pow(v, 0.3f)*(0.5/(std::pow(0.5, 0.3)));
  } else {
    return 0.5+std::pow(v-0.5, 2.0f)*(0.5/(std::pow(0.5, 2.0f)));
  }
}

void ArrangementArea::paintPlayCursor(juce::Graphics& g) {
  g.setColour(cursorColor);
  // in the cursor moving phase, we avoid waiting tracks locks
  // by using the mouse value
  if (!isMovingCursor) {
    lastPlayCursorPosition = ((sampleManager.getNextReadPosition()-viewPosition)/viewScale);
    g.fillRect(
      lastPlayCursorPosition-(PLAYCURSOR_WIDTH>>1),
      0,
      PLAYCURSOR_WIDTH,
      FREQTIME_VIEW_HEIGHT
    );
  } else {
    lastPlayCursorPosition = lastMouseX;
    g.fillRect(
      lastPlayCursorPosition-(PLAYCURSOR_WIDTH>>1),
      0,
      PLAYCURSOR_WIDTH,
      FREQTIME_VIEW_HEIGHT
    );
  }
}

void ArrangementArea::mouseDown(const juce::MouseEvent& jme) {
  // saving last mouse position
  juce::Point<int> position = jme.getPosition();
  lastMouseX = position.getX();
  lastMouseY = position.getY();

  // if the mouse was clicked
  if (jme.mouseWasClicked()) {
    if (jme.mods.isMiddleButtonDown()) {
      handleMiddleButterDown(jme);
    }
    if (jme.mods.isLeftButtonDown()) {
      handleLeftButtonDown(jme);
    }
  }
}

void ArrangementArea::handleMiddleButterDown(const juce::MouseEvent& jme) {
  // handle resize/mode mode activation
  if (!isMovingCursor && !isResizing && trackMovingInitialPosition==-1) {
    isResizing = true;
  }
}

void ArrangementArea::handleLeftButtonDown(const juce::MouseEvent& jme) {
  size_t clickedTrack;
  // handle click when in default mode
  if(!isMovingCursor && !isResizing) {
    // if we're clicking around a cursor
    if(abs(lastMouseX-lastPlayCursorPosition)<PLAYCURSOR_GRAB_WIDTH) {
      // enter cursor moving mode
      isMovingCursor = true;
      // else, see if we're clicking tracks for selection
    } else if (!isMovingCursor && trackMovingInitialPosition==-1) {
      clickedTrack = getTrackClicked(jme);
      if(clickedTrack!=-1) {
        // if ctrl is not pressed, we clear selection set
        if (!jme.mods.isCtrlDown()) {
          selectedTracks.clear();
        }
        selectedTracks.insert(clickedTrack);
        repaint();
      // if clicking in the void, unselected everything
      } else {
        if(!selectedTracks.empty()) {
          selectedTracks.clear();
          repaint();
        }
      }
    }
  }
}

size_t ArrangementArea::getTrackClicked(const juce::MouseEvent& jme) {
  size_t nTracks = sampleManager.getNumTracks();

  // reference to the samplePlayer we are drawing in the loop
  SamplePlayer* sp;

  int64_t trackPosition;

  // for each track
  for (size_t i = 0; i < nTracks; i++) {
    // get a reference to the sample
    sp = sampleManager.getTrack(i);
    // skip nullptr in tracks list (should not happen)
    if (sp == nullptr) {
      continue;
    }
    // get its lock
    const juce::SpinLock::ScopedLockType lock(sp->playerMutex);
    
    trackPosition = (sp->getEditingPosition() - viewPosition) / viewScale;

    // if it's inbound, return the index
    if(lastMouseX > trackPosition && lastMouseX < trackPosition+(sp->getLength()/viewScale)) {
      return i;
    } 
  }

  return -1;
}

void ArrangementArea::mouseUp(const juce::MouseEvent& jme) {
  // saving last mouse position
  juce::Point<int> position = jme.getPosition();
  lastMouseX = position.getX();
  lastMouseY = position.getY();

  // pass event to handlers
  if (jme.mods.isLeftButtonDown()) {
    handleLeftButtonUp(jme);
  }
}

void ArrangementArea::handleLeftButtonUp(const juce::MouseEvent& jme) {
  // handle cursor mode relieving
  if(isMovingCursor) {
    isMovingCursor = false;
    sampleManager.setNextReadPosition(viewPosition + lastMouseX*viewScale);
  }
}

void ArrangementArea::mouseDrag(const juce::MouseEvent& jme) {
  juce::Point<int> newPosition = jme.getPosition();

  // some preallocated values
  int pixelMovement, frameMovement;
  int64_t oldViewScale, oldViewPosition;
  oldViewPosition = viewPosition;
  oldViewScale = viewScale;
  bool viewUpdated = false;

  // handle resize mode
  if (isResizing) {

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
    // save some repait by comparing viewScale and viewPosition
    if (oldViewPosition != viewPosition || oldViewScale != viewScale) {
      viewUpdated = true;
    }
  }

  // saving last mouse position
  lastMouseX = newPosition.getX();
  lastMouseY = newPosition.getY();

  // if updated view or in cursor moving mode, repaint
  if(viewUpdated || isMovingCursor) {
    repaint();
  }
}

void ArrangementArea::mouseMove(const juce::MouseEvent& jme) {

  // saving last mouse position
  juce::Point<int> newPosition = jme.getPosition();
  lastMouseX = newPosition.getX();
  lastMouseY = newPosition.getY();
  
  // if we are in resize mode and middle mouse button is not pressed
  if (isResizing && !jme.mods.isMiddleButtonDown()) {
    // save that we are not in resize mode anymore
    isResizing = false;
  }

  // if in drag mode, repaint on movement
  if (trackMovingInitialPosition!=-1) {
    repaint();
  }
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
  int64_t framePos = viewPosition + (x * viewScale);
  // we try to load the samples
  for (int i = 0; i < files.size(); i++) {
    int id = sampleManager.addSample(files[i], framePos);
    // on failures, abort
    if (id == -1) {
      // notify error
      notificationArea.notifyError("unable to load " + files[i]);
      continue;
    }
  }
  if (refreshNeeded) {
    repaint();
  }
}

bool ArrangementArea::keyPressed(const juce::KeyPress &key) {
  // if the space key is pressed, play or pause
  if(key==juce::KeyPress::spaceKey) {
    if(sampleManager.isCursorPlaying()) {
      sampleManager.stopPlayback();
    } else if (!isMovingCursor) {
      sampleManager.startPlayback();
    }
  } else if (key==juce::KeyPress::createFromDescription(KEYMAP_DRAG_MODE)) {
    // if pressing d and not in any mode, start dragging
    if(!isResizing && trackMovingInitialPosition==-1 && !selectedTracks.empty()) {
      trackMovingInitialPosition = lastMouseX;
    }
  } else if (key==juce::KeyPress::createFromDescription(KEYMAP_DELETE_SELECTION)) {
    // if pressing x and not in any mode, delete selected tracks
    if(!isResizing && trackMovingInitialPosition==-1 && !selectedTracks.empty()) {
      deleteSelectedTracks();
    }
  }
  // do not intercept the signal and pass it around
  return false;
}

void ArrangementArea::deleteSelectedTracks() {
  // for each selected track
  std::set<std::size_t>::iterator it = selectedTracks.begin();
  while (it != selectedTracks.end()) {
    // delete it
    SamplePlayer * deletedSp = sampleManager.deleteTrack(*it);
    delete deletedSp;
    it++;
  }
  // clear selection and redraw
  selectedTracks.clear();
  repaint();
}

bool ArrangementArea::keyStateChanged(bool isKeyDown) {
  // if in drag mode
  if(trackMovingInitialPosition!=-1) {
    // if the D key is not pressed anymore
    if(!juce::KeyPress::isKeyCurrentlyDown(
      juce::KeyPress::createFromDescription(KEYMAP_DRAG_MODE).getKeyCode()
      )
    ) {
      // update tracks position, get out of drag mode, and repaint
      size_t nTracks = sampleManager.getNumTracks();
      SamplePlayer* sp;
      int64_t trackPosition;
      int64_t dragDistance = (lastMouseX - trackMovingInitialPosition)*viewScale; 
      // for each track
      for (size_t i = 0; i < nTracks; i++) {
        if (auto search = selectedTracks.find(i); search != selectedTracks.end()) {
            // get a reference to the sample
            sp = sampleManager.getTrack(i);
            // skip nullptr in tracks list (should not happen)
            if (sp == nullptr) {
              continue;
            }
            // get its lock
            const juce::SpinLock::ScopedLockType lock(sp->playerMutex);
            // get the old position
            trackPosition = sp->getEditingPosition();
            trackPosition += dragDistance;
            sp->move(trackPosition);
        }
      }
      trackMovingInitialPosition = -1;
      repaint();
    }
  }
  return false;
}

// Sigmoid activation function to try to increase contract in fft
float ArrangementArea::sigmoid (float val) {
    return 1 / (1 + exp(-val));
}