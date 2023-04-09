#include "ArrangementArea.h"

#include <cmath>
#include <iostream>
#include <stdexcept>

#include "../OpenGL/FreqviewShaders.h"
#include "../OpenGL/GLInfoLogger.h"
#include "juce_opengl/opengl/juce_gl.h"

// NOTE:
// We often mention "audio frames" in the code for a float of signal intensity.
// Standard name is "audio samples" but this can be
// confused with the audio samples we are supposed to edit
// with this software so we say "frames" sometimes.

// OpenGL examples:
// https://learnopengl.com/Getting-started/Hello-Triangle
// https://learnopengl.com/Getting-started/Shaders
// https://learnopengl.com/Getting-started/Textures
// https://medium.com/@Im_Jimmi/using-opengl-for-2d-graphics-in-a-juce-plug-in-24aa82f634ff
//

//==============================================================================
ArrangementArea::ArrangementArea(SampleManager& sm, NotificationArea& na)
    : sampleManager(sm), notificationArea(na) {
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
  fftBlockHeight = float(FREQTIME_VIEW_HEIGHT - FREQTIME_VIEW_INNER_MARGINS) /
                   float(FREQVIEW_SAMPLE_FFT_SIZE << 1);
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

  shadersCompiled = false;

  // Indicates that no part of this Component is transparent.
  setOpaque(true);

  openGLContext.setRenderer(this);
  openGLContext.setContinuousRepainting(false);
  openGLContext.attachTo(*this);

  // register the callback to register newly created samples
  sampleManager.addUiSampleCallback = [this](SamplePlayer* sp) {
    addNewSample(sp);
  };
  sampleManager.disableUiSampleCallback = [this](int index) {
    samples[index].disable();
  };
}

ArrangementArea::~ArrangementArea() { openGLContext.detach(); }

void ArrangementArea::paint(juce::Graphics& g) {
  // get the window width
  bounds = g.getClipBounds();

  // NOTE: this function draws on top of openGL

  // draw nothing if the windows is too small
  if (bounds.getWidth() < FREQVIEW_MIN_WIDTH ||
      bounds.getHeight() < FREQVIEW_MIN_HEIGHT) {
    g.fillAll(juce::Colour(20, 20, 20));
    return;
  }

  // paint the play cursor
  paintPlayCursor(g);
}

void ArrangementArea::resized() {
  // This is called when the MainComponent is resized.
  bounds = getBounds();
  if (shadersCompiled) {
    updateShadersPositionUniforms();
  }
}

void ArrangementArea::newOpenGLContextCreated() {
  std::cerr << "Initializing OpenGL context..." << std::endl;
  // Instanciate an instance of OpenGLShaderProgram
  texturedPositionedShader.reset(new juce::OpenGLShaderProgram(openGLContext));
  backgroundGridShader.reset(new juce::OpenGLShaderProgram(openGLContext));
  // Compile and link the shader
  if (buildShaders()) {
    shadersCompiled = true;

    std::cerr << "Sucessfully compiled OpenGL shaders" << std::endl;

    // we push dummy values because the bounds may not have been set yet
    texturedPositionedShader->use();
    texturedPositionedShader->setUniform("ourTexture", 0);

    updateShadersPositionUniforms(true);

    // log some info about openGL version and all
    logOpenGLInfoCallback(openGLContext);
    // enable the error logging
    enableOpenGLErrorLogging();

    // initialize background openGL objects
    backgroundGrid.registerGlObjects();

  } else {
    std::cerr << "FATAL: Unable to compile OpenGL Shaders" << std::endl;
    juce::JUCEApplicationBase::quit();
  }
}

bool ArrangementArea::buildShaders() {
  bool builtTexturedShader = buildShader(
      texturedPositionedShader, sampleVertexShader, sampleFragmentShader);
  if (!builtTexturedShader) {
    std::cerr << "Failed to build textured positioned shaders" << std::endl;
    return false;
  }
  bool builtColoredShader =
      buildShader(backgroundGridShader, gridBackgroundVertexShader,
                  gridBackgroundFragmentShader);
  if (!builtColoredShader) {
    std::cerr << "Failed to build coloured positioned shaders" << std::endl;
    return false;
  }
  return true;
}

bool ArrangementArea::buildShader(
    std::unique_ptr<juce::OpenGLShaderProgram>& sh, std::string vertexShader,
    std::string fragmentShader) {
  return sh->addVertexShader(vertexShader) &&
         sh->addFragmentShader(fragmentShader) && sh->link();
}

void ArrangementArea::updateShadersPositionUniforms(bool fromGlThread) {
  // send the new view positions to opengl thread
  if (!fromGlThread) {
    openGLContext.executeOnGLThread(
        [this](juce::OpenGLContext&) { alterShadersPositions(); }, false);
  } else {
    alterShadersPositions();
  }
}

void ArrangementArea::alterShadersPositions() {
  texturedPositionedShader->use();
  texturedPositionedShader->setUniform("viewPosition", (GLfloat)viewPosition);
  texturedPositionedShader->setUniform(
      "viewWidth", (GLfloat)(bounds.getWidth() * viewScale));

  updateGridPixelValues();

  backgroundGridShader->use();
  backgroundGridShader->setUniform("grid0PixelShift", (GLint)grid0PixelShift);
  backgroundGridShader->setUniform("grid0PixelWidth", (GLint)grid0PixelWidth);

  backgroundGridShader->setUniform("viewHeightPixels",
                                   (GLfloat)(bounds.getHeight()));
}

void ArrangementArea::updateGridPixelValues() {
  grid0FrameWidth = ((60 * 44100) / tempo);
  grid0PixelWidth = grid0FrameWidth / viewScale;
  grid0PixelShift =
      (grid0FrameWidth - (viewPosition % grid0FrameWidth)) / viewScale;
}

void ArrangementArea::renderOpenGL() {
  // enable the damn blending
  juce::gl::glEnable(juce::gl::GL_BLEND);
  juce::gl::glBlendFunc(juce::gl::GL_SRC_ALPHA,
                        juce::gl::GL_ONE_MINUS_SRC_ALPHA);

  juce::gl::glClearColor(0.078f, 0.078f, 0.078f, 1.0f);
  juce::gl::glClear(juce::gl::GL_COLOR_BUFFER_BIT);

  backgroundGridShader->use();
  backgroundGrid.drawGlObjects();

  texturedPositionedShader->use();

  for (int i = 0; i < samples.size(); i++) {
    samples[i].drawGlObjects();
  }
}

void ArrangementArea::openGLContextClosing() {}

void ArrangementArea::addNewSample(SamplePlayer* sp) {
  // create graphic objects from the sample
  samples.push_back(SampleGraphicModel(sp));
  // send the data to the GPUs from the OpenGL thread
  openGLContext.executeOnGLThread(
      [this](juce::OpenGLContext& c) {
        samples[samples.size() - 1].registerGlObjects();
      },
      true);
}

void ArrangementArea::updateSamplePosition(int index, juce::int64 position) {
  // TODO
}

void ArrangementArea::paintPlayCursor(juce::Graphics& g) {
  g.setColour(cursorColor);
  // in the cursor moving phase, we avoid waiting tracks locks
  // by using the mouse value
  if (!isMovingCursor) {
    lastPlayCursorPosition =
        ((sampleManager.getNextReadPosition() - viewPosition) / viewScale);
    g.fillRect(lastPlayCursorPosition - (PLAYCURSOR_WIDTH >> 1), 0,
               PLAYCURSOR_WIDTH, FREQTIME_VIEW_HEIGHT);
  } else {
    lastPlayCursorPosition = lastMouseX;
    g.fillRect(lastPlayCursorPosition - (PLAYCURSOR_WIDTH >> 1), 0,
               PLAYCURSOR_WIDTH, FREQTIME_VIEW_HEIGHT);
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
  if (!isMovingCursor && !isResizing && trackMovingInitialPosition == -1) {
    isResizing = true;
  }
}

void ArrangementArea::handleLeftButtonDown(const juce::MouseEvent& jme) {
  size_t clickedTrack;
  // handle click when in default mode
  if (!isMovingCursor && !isResizing) {
    // if we're clicking around a cursor
    if (abs(lastMouseX - lastPlayCursorPosition) < PLAYCURSOR_GRAB_WIDTH) {
      // enter cursor moving mode
      isMovingCursor = true;
      // else, see if we're clicking tracks for selection
    } else if (!isMovingCursor && trackMovingInitialPosition == -1) {
      clickedTrack = getTrackClicked(jme);
      if (clickedTrack != -1) {
        // if ctrl is not pressed, we clear selection set
        if (!jme.mods.isCtrlDown()) {
          selectedTracks.clear();
        }
        selectedTracks.insert(clickedTrack);
        repaint();
        // if clicking in the void, unselected everything
      } else {
        if (!selectedTracks.empty()) {
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
    if (lastMouseX > trackPosition &&
        lastMouseX < trackPosition + (sp->getLength() / viewScale)) {
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
  if (isMovingCursor) {
    isMovingCursor = false;
    sampleManager.setNextReadPosition(viewPosition + lastMouseX * viewScale);
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
  if (viewUpdated || isMovingCursor) {
    updateShadersPositionUniforms();
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
  if (trackMovingInitialPosition != -1) {
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

bool ArrangementArea::keyPressed(const juce::KeyPress& key) {
  // if the space key is pressed, play or pause
  if (key == juce::KeyPress::spaceKey) {
    if (sampleManager.isCursorPlaying()) {
      sampleManager.stopPlayback();
    } else if (!isMovingCursor) {
      sampleManager.startPlayback();
    }
  } else if (key == juce::KeyPress::createFromDescription(KEYMAP_DRAG_MODE)) {
    // if pressing d and not in any mode, start dragging
    if (!isResizing && trackMovingInitialPosition == -1 &&
        !selectedTracks.empty()) {
      trackMovingInitialPosition = lastMouseX;
    }
  } else if (key ==
             juce::KeyPress::createFromDescription(KEYMAP_DELETE_SELECTION)) {
    // if pressing x and not in any mode, delete selected tracks
    if (!isResizing && trackMovingInitialPosition == -1 &&
        !selectedTracks.empty()) {
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
    SamplePlayer* deletedSp = sampleManager.deleteTrack(*it);
    delete deletedSp;
    it++;
  }
  // clear selection and redraw
  selectedTracks.clear();
  repaint();
}

bool ArrangementArea::keyStateChanged(bool isKeyDown) {
  // if in drag mode
  if (trackMovingInitialPosition != -1) {
    // if the D key is not pressed anymore
    if (!juce::KeyPress::isKeyCurrentlyDown(
            juce::KeyPress::createFromDescription(KEYMAP_DRAG_MODE)
                .getKeyCode())) {
      // update tracks position, get out of drag mode, and repaint
      size_t nTracks = sampleManager.getNumTracks();
      SamplePlayer* sp;
      int64_t trackPosition;
      int64_t dragDistance =
          (lastMouseX - trackMovingInitialPosition) * viewScale;
      // for each track
      for (size_t i = 0; i < nTracks; i++) {
        if (auto search = selectedTracks.find(i);
            search != selectedTracks.end()) {
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

bool ArrangementArea::isInterestedInDragSource(
    const SourceDetails& dragSourceDetails) {
  juce::String filename = dragSourceDetails.description.toString();
  if (filename.startsWith("file:")) {
    if (filename.endsWith(".mp3") || filename.endsWith(".wav")) {
      return true;
    }
  }
  return false;
}

void ArrangementArea::itemDropped(const SourceDetails& dragSourceDetails) {
  int x = dragSourceDetails.localPosition.getX();
  // converts x to an valid position in audio frame
  int64_t framePos = viewPosition + (x * viewScale);
  // we try to load the sample
  juce::String filename =
      dragSourceDetails.description.toString().replaceFirstOccurrenceOf("file:",
                                                                        "");
  int id = sampleManager.addSample(filename, framePos);
  // on failures, abort
  if (id == -1) {
    // notify error
    notificationArea.notifyError("unable to load " + filename);
    return;
  }

  repaint();
}