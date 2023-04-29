#include "ArrangementArea.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
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
ArrangementArea::ArrangementArea(MixingBus &mb, NotificationArea &na, ActivityManager &am)
    : mixingBus(mb), notificationArea(na), activityManager(am)
{
    // save reference to the sample manager
    // initialize grid and position
    viewPosition = 0;
    viewScale = 100;
    lastMouseX = 0;
    lastMouseY = 0;
    lastPlayCursorPosition = 0;
    trackMovingInitialPosition = -1;

    tempo = 120;

    // play cursor color
    cursorColor = juce::Colour(240, 240, 240);
    // enable keyboard events
    setWantsKeyboardFocus(true);

    shadersCompiled = false;
    recentlyDuplicated = false;

    // Indicates that no part of this Component is transparent.
    setOpaque(true);

    openGLContext.setRenderer(this);
    openGLContext.setContinuousRepainting(false);
    openGLContext.attachTo(*this);

    // register the callback to register newly created samples
    mixingBus.addUiSampleCallback = [this](SamplePlayer *sp, SampleImportTask task) { displaySample(sp, task); };
    mixingBus.disableUiSampleCallback = [this](int index) { samples[index]->disable(); };
}

ArrangementArea::~ArrangementArea()
{
    openGLContext.detach();
}

void ArrangementArea::paint(juce::Graphics &g)
{
    // get the window width
    bounds = g.getClipBounds();

    // NOTE: this function draws on top of openGL

    // draw nothing if the windows is too small
    if (bounds.getWidth() < FREQVIEW_MIN_WIDTH || bounds.getHeight() < FREQVIEW_MIN_HEIGHT)
    {
        g.fillAll(juce::Colour(20, 20, 20));
        return;
    }

    paintSelection(g);
    paintPlayCursor(g);
    paintLabels(g);
}

void ArrangementArea::paintSelection(juce::Graphics &g)
{
    // iterate over tracks and draw borders around them
    std::set<size_t>::iterator itr;
    SampleAreaRectangle currentSampleBorders;

    // empty buffer of selected samples borders
    selectedSamplesCoordsBuffer.clear();

    g.setColour(COLOR_SAMPLE_BORDER);
    for (itr = selectedTracks.begin(); itr != selectedTracks.end(); itr++)
    {
        // ignore deleted selected tracks
        if (mixingBus.getTrack(*itr) != nullptr)
        {
            auto samplesRects = samples[*itr]->getPixelBounds(viewPosition, viewScale, bounds.getHeight());

            for (size_t i = 0; i < samplesRects.size(); i++)
            {
                // reject what's not on screen
                if (samplesRects[i].getX() + samplesRects[i].getWidth() < 0 ||
                    samplesRects[i].getX() > bounds.getWidth())
                {
                    continue;
                }

                // if we reach here it's on screen, we draw and save it
                currentSampleBorders = samplesRects[i];

                currentSampleBorders.setSampleIndex(*itr);
                currentSampleBorders.setNumParts(samplesRects.size());
                currentSampleBorders.setPartId(i);

                g.drawRoundedRectangle(currentSampleBorders, 4, 1.7);
                selectedSamplesCoordsBuffer.push_back(SampleAreaRectangle(currentSampleBorders));
            }
        }
    }

    selectedSamplesCoords.swap(selectedSamplesCoordsBuffer);
}

juce::Optional<SampleBorder> ArrangementArea::mouseOverSelectionBorder()
{

    // if we're already over a label, do nothing
    for (size_t i = 0; i < onScreenLabelsPixelsCoords.size(); i++)
    {
        if (onScreenLabelsPixelsCoords[i].contains(lastMouseX, lastMouseY))
        {
            return juce::Optional<SampleBorder>(juce::nullopt);
        }
    }

    // iterate over on screen selected samples
    for (size_t i = 0; i < selectedSamplesCoords.size(); i++)
    {
        // if our click is in bounds of this sample
        if (selectedSamplesCoords[i].expanded(PLAYCURSOR_GRAB_WIDTH).contains(lastMouseX, lastMouseY))
        {

            SampleDirection direction = LOW_FREQS_TO_TOP;
            if (selectedSamplesCoords[i].getPartId() == 0)
            {
                direction = LOW_FREQS_TO_BOTTOM;
            }

            // switch on the border
            if (abs(selectedSamplesCoords[i].getX() - lastMouseX) < PLAYCURSOR_GRAB_WIDTH)
            {
                return juce::Optional<SampleBorder>(
                    SampleBorder(selectedSamplesCoords[i].getSampleIndex(), BORDER_LEFT, direction));
            }

            if (abs(selectedSamplesCoords[i].getX() + selectedSamplesCoords[i].getWidth() - lastMouseX) <
                PLAYCURSOR_GRAB_WIDTH)
            {
                return juce::Optional<SampleBorder>(
                    SampleBorder(selectedSamplesCoords[i].getSampleIndex(), BORDER_RIGHT, direction));
            }

            if (abs(selectedSamplesCoords[i].getY() - lastMouseY) < PLAYCURSOR_GRAB_WIDTH)
            {
                return juce::Optional<SampleBorder>(
                    SampleBorder(selectedSamplesCoords[i].getSampleIndex(), BORDER_UPPER, direction));
            }

            if (abs(selectedSamplesCoords[i].getY() + selectedSamplesCoords[i].getHeight() - lastMouseY) <
                PLAYCURSOR_GRAB_WIDTH)
            {
                return juce::Optional<SampleBorder>(
                    SampleBorder(selectedSamplesCoords[i].getSampleIndex(), BORDER_LOWER, direction));
            }
        }
    }
    return juce::Optional<SampleBorder>(juce::nullopt);
}

void ArrangementArea::paintLabels(juce::Graphics &g)
{
    // prefix notes:
    // Frame: coordinates in global audio frames position
    // Pixel: coordinates in pixels

    // clear the list of previous labels (to be later swapped)
    onScreenLabelsPixelsCoordsBuffer.clear();

    SampleAreaRectangle currentLabelPixelsCoords;

    int currentSampleLeftSideFrame, currentSampleRightSideFrame;

    int viewLeftMostFrame = viewPosition;
    int viewRightMostFrame = viewPosition + (bounds.getWidth() * viewScale);

    for (int i = 0; i < samples.size(); i++)
    {
        if (samples[i]->isDisabled())
        {
            continue;
        }
        currentSampleLeftSideFrame = samples[i]->getFramePosition();
        currentSampleRightSideFrame = currentSampleLeftSideFrame + samples[i]->getFrameLength();

        // ignore samples that are out of bound
        if (currentSampleLeftSideFrame < viewRightMostFrame && currentSampleRightSideFrame > viewLeftMostFrame)
        {
            // put the labels limits in bounds
            if (currentSampleLeftSideFrame < viewLeftMostFrame)
            {
                currentSampleLeftSideFrame = viewLeftMostFrame;
            }
            if (currentSampleRightSideFrame > viewRightMostFrame)
            {
                currentSampleRightSideFrame = viewRightMostFrame;
            }

            // translate into pixel coordinates and prevent position conflicts
            currentLabelPixelsCoords = addLabelAndPreventOverlaps(
                onScreenLabelsPixelsCoordsBuffer, (currentSampleLeftSideFrame - viewPosition) / viewScale,
                (currentSampleRightSideFrame - viewPosition) / viewScale, i);
            if (currentLabelPixelsCoords.getSampleIndex() != -1)
            {
                paintSampleLabel(g, currentLabelPixelsCoords, i);
            }
        }
    }

    // save the labels displayed on screen
    onScreenLabelsPixelsCoords.swap(onScreenLabelsPixelsCoordsBuffer);
}

SampleAreaRectangle ArrangementArea::addLabelAndPreventOverlaps(std::vector<SampleAreaRectangle> &existingLabels,
                                                                int leftSidePixelCoords, int rightSidePixelCoords,
                                                                int sampleIndex)
{
    // the pixel coordinates of the upcoming label
    SampleAreaRectangle pixelLabelRect;
    pixelLabelRect.setWidth(rightSidePixelCoords - leftSidePixelCoords);
    pixelLabelRect.setWidth(juce::jmin(pixelLabelRect.getWidth(), FREQVIEW_LABELS_MAX_WIDTH));
    pixelLabelRect.setX(leftSidePixelCoords);
    pixelLabelRect.setHeight(FREQVIEW_LABEL_HEIGHT);

    // reduce the width if there's unused space
    int textPixelWidth = juce::Font().getStringWidth(taxonomyManager.getSampleName(sampleIndex));
    if (pixelLabelRect.getWidth() > textPixelWidth + FREQVIEW_LABELS_MARGINS * 2 + pixelLabelRect.getHeight())
    {
        pixelLabelRect.setWidth(textPixelWidth + FREQVIEW_LABELS_MARGINS * 2 + pixelLabelRect.getHeight());
    }

    int maxTrials = bounds.getHeight() / FREQVIEW_LABEL_HEIGHT;

    // we will keep moving it untill there's no overlap to other labels
    // and it lies over its samples area
    float origin = float(bounds.getHeight() >> 1);
    pixelLabelRect.setY(origin - (FREQVIEW_LABEL_HEIGHT >> 1));
    int trials = 0;
    while (rectangleIntersects(pixelLabelRect, existingLabels) || !overlapSampleArea(pixelLabelRect, sampleIndex, 4))
    {
        if (trials % 2 == 0)
        {
            pixelLabelRect.setY(origin + ((trials >> 1) + 1) * FREQVIEW_LABEL_HEIGHT - (FREQVIEW_LABEL_HEIGHT >> 1));
        }
        else
        {
            pixelLabelRect.setY(origin - ((trials >> 1) + 1) * FREQVIEW_LABEL_HEIGHT - (FREQVIEW_LABEL_HEIGHT >> 1));
        }
        trials++;

        if (trials >= maxTrials)
        {
            return pixelLabelRect;
        }
    }

    // skip label if it's too small by not giving it an id and not pushing it to
    // the list of labels
    if (pixelLabelRect.getWidth() < 2.0f * pixelLabelRect.getHeight())
    {
        return pixelLabelRect;
    }

    // add and return new label with no overlap
    pixelLabelRect.setSampleIndex(sampleIndex);
    existingLabels.push_back(pixelLabelRect);
    return pixelLabelRect;
}

bool ArrangementArea::rectangleIntersects(SampleAreaRectangle &target, std::vector<SampleAreaRectangle> &rectangles)
{
    for (int i = 0; i < rectangles.size(); i++)
    {
        if (target.intersects(rectangles[i]))
        {
            return true;
        }
    }
    return false;
}

bool ArrangementArea::overlapSampleArea(SampleAreaRectangle &rect, int sampleIndex, int margin)
{
    auto sampleRects = samples[sampleIndex]->getPixelBounds(viewPosition, viewScale, bounds.getHeight());

    for (size_t i = 0; i < sampleRects.size(); i++)
    {
        if (sampleRects[i].expanded(margin, margin).contains(rect))
        {
            return true;
        }
    }
    return false;
}

void ArrangementArea::paintSampleLabel(juce::Graphics &g, juce::Rectangle<float> &box, int index)
{
    g.setColour(juce::Colour::fromFloatRGBA(0.0f, 0.0f, 0.0f, 0.5f));
    g.fillRoundedRectangle(box, FREQVIEW_LABELS_CORNER_ROUNDING);

    g.setColour(COLOR_LABELS_BORDER);

    // different border width depending on if selected or not
    if (selectedTracks.find(index) != selectedTracks.end())
    {
        g.drawRoundedRectangle(box, FREQVIEW_LABELS_CORNER_ROUNDING, FREQVIEW_LABELS_BORDER_THICKNESS);
    }
    else
    {
        g.drawRoundedRectangle(box, FREQVIEW_LABELS_CORNER_ROUNDING, FREQVIEW_LABELS_BORDER_THICKNESS / 2.0f);
    }

    auto reducedBox = box.reduced(FREQVIEW_LABELS_MARGINS, FREQVIEW_LABELS_MARGINS);

    g.setColour(taxonomyManager.getSampleColor(index));
    g.fillRect(reducedBox.withWidth(box.getHeight() - FREQVIEW_LABELS_MARGINS));

    g.setColour(COLOR_LABELS_BORDER);
    g.drawText(taxonomyManager.getSampleName(index),
               reducedBox.translated(box.getHeight(), 0).withWidth(reducedBox.getWidth() - box.getHeight()),
               juce::Justification::centredLeft, true);
}

void ArrangementArea::resized()
{
    // This is called when the MainComponent is resized.
    bounds = getBounds();
    if (shadersCompiled)
    {
        updateShadersPositionUniforms();
    }
}

void ArrangementArea::newOpenGLContextCreated()
{
    std::cerr << "Initializing OpenGL context..." << std::endl;
    // Instanciate an instance of OpenGLShaderProgram
    texturedPositionedShader.reset(new juce::OpenGLShaderProgram(openGLContext));
    backgroundGridShader.reset(new juce::OpenGLShaderProgram(openGLContext));
    // Compile and link the shader
    if (buildShaders())
    {
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
    }
    else
    {
        std::cerr << "FATAL: Unable to compile OpenGL Shaders" << std::endl;
        juce::JUCEApplicationBase::quit();
    }
}

bool ArrangementArea::buildShaders()
{
    bool builtTexturedShader = buildShader(texturedPositionedShader, sampleVertexShader, sampleFragmentShader);
    if (!builtTexturedShader)
    {
        std::cerr << "Failed to build textured positioned shaders" << std::endl;
        return false;
    }
    bool builtColoredShader =
        buildShader(backgroundGridShader, gridBackgroundVertexShader, gridBackgroundFragmentShader);
    if (!builtColoredShader)
    {
        std::cerr << "Failed to build coloured positioned shaders" << std::endl;
        return false;
    }
    return true;
}

bool ArrangementArea::buildShader(std::unique_ptr<juce::OpenGLShaderProgram> &sh, std::string vertexShader,
                                  std::string fragmentShader)
{
    return sh->addVertexShader(vertexShader) && sh->addFragmentShader(fragmentShader) && sh->link();
}

void ArrangementArea::updateShadersPositionUniforms(bool fromGlThread)
{
    // send the new view positions to opengl thread
    if (!fromGlThread)
    {
        openGLContext.executeOnGLThread([this](juce::OpenGLContext &) { alterShadersPositions(); }, false);
    }
    else
    {
        alterShadersPositions();
    }
}

void ArrangementArea::alterShadersPositions()
{
    texturedPositionedShader->use();
    texturedPositionedShader->setUniform("viewPosition", (GLfloat)viewPosition);
    texturedPositionedShader->setUniform("viewWidth", (GLfloat)(bounds.getWidth() * viewScale));

    updateGridPixelValues();

    backgroundGridShader->use();
    backgroundGridShader->setUniform("grid0PixelShift", (GLint)grid0PixelShift);
    backgroundGridShader->setUniform("grid0PixelWidth", (GLfloat)grid0PixelWidth);

    backgroundGridShader->setUniform("grid1PixelShift", (GLint)grid1PixelShift);
    backgroundGridShader->setUniform("grid1PixelWidth", (GLfloat)grid1PixelWidth);

    backgroundGridShader->setUniform("grid2PixelShift", (GLint)grid2PixelShift);
    backgroundGridShader->setUniform("grid2PixelWidth", (GLfloat)grid2PixelWidth);

    backgroundGridShader->setUniform("viewHeightPixels", (GLfloat)(bounds.getHeight()));
}

void ArrangementArea::updateGridPixelValues()
{
    int framesPerMinutes = (60 * 44100);

    grid0FrameWidth = (float(framesPerMinutes) / float(tempo));
    grid0PixelWidth = grid0FrameWidth / float(viewScale);
    grid0PixelShift = (grid0FrameWidth - (viewPosition % int(grid0FrameWidth))) / viewScale;

    grid1FrameWidth = (float(framesPerMinutes) / float(tempo * 4));
    grid1PixelWidth = grid1FrameWidth / float(viewScale);
    grid1PixelShift = (grid1FrameWidth - (viewPosition % int(grid1FrameWidth))) / viewScale;

    grid2FrameWidth = (float(framesPerMinutes) / float(tempo * 16));
    grid2PixelWidth = grid2FrameWidth / float(viewScale);
    grid2PixelShift = (grid2FrameWidth - (viewPosition % int(grid2FrameWidth))) / viewScale;
}

void ArrangementArea::renderOpenGL()
{
    // enable the damn blending
    juce::gl::glEnable(juce::gl::GL_BLEND);
    juce::gl::glBlendFunc(juce::gl::GL_SRC_ALPHA, juce::gl::GL_ONE_MINUS_SRC_ALPHA);

    juce::gl::glClearColor(0.078f, 0.078f, 0.078f, 1.0f);
    juce::gl::glClear(juce::gl::GL_COLOR_BUFFER_BIT);

    backgroundGridShader->use();
    backgroundGrid.drawGlObjects();

    texturedPositionedShader->use();

    for (int i = 0; i < samples.size(); i++)
    {
        samples[i]->drawGlObjects();
    }
}

void ArrangementArea::openGLContextClosing()
{
}

void ArrangementArea::displaySample(SamplePlayer *sp, SampleImportTask task)
{
    // create graphic objects from the sample
    SampleGraphicModel *sampleRef =
        new SampleGraphicModel(sp, taxonomyManager.getSampleColor(task.getAllocatedIndex()));
    samples.push_back(sampleRef);
    // fatal error if sample ids are different
    if ((samples.size() - 1) != task.getAllocatedIndex())
    {
        std::cerr << "FATAL: Sample ids don't match in Mixbus and ArrangementArea" << std::endl;
        juce::JUCEApplicationBase::quit();
    }
    // assign default name to sample
    taxonomyManager.setSampleName(task.getAllocatedIndex(), sp->getFileName());
    // send the data to the GPUs from the OpenGL thread
    openGLContext.executeOnGLThread(
        [this](juce::OpenGLContext &c) { samples[samples.size() - 1]->registerGlObjects(); }, true);
    // if it's a copy, set the group and update the color
    if (task.isDuplication())
    {
        taxonomyManager.copyTaxonomy(task.getDuplicateTargetId(), task.getAllocatedIndex());
        syncSampleColor(task.getAllocatedIndex());
        selectedTracks.insert(task.getAllocatedIndex());
    }
}

void ArrangementArea::paintPlayCursor(juce::Graphics &g)
{
    g.setColour(cursorColor);
    // in the cursor moving phase, we avoid waiting tracks locks
    // by using the mouse value
    if (activityManager.getAppState().getUiState() != UI_STATE_CURSOR_MOVING)
    {
        lastPlayCursorPosition = ((mixingBus.getNextReadPosition() - viewPosition) / viewScale);
        g.fillRect(lastPlayCursorPosition - (PLAYCURSOR_WIDTH >> 1), 0, PLAYCURSOR_WIDTH, FREQTIME_VIEW_HEIGHT);
    }
    else
    {
        lastPlayCursorPosition = lastMouseX;
        g.fillRect(lastPlayCursorPosition - (PLAYCURSOR_WIDTH >> 1), 0, PLAYCURSOR_WIDTH, FREQTIME_VIEW_HEIGHT);
    }
}

void ArrangementArea::mouseDown(const juce::MouseEvent &jme)
{
    // saving last mouse position
    juce::Point<int> position = jme.getPosition();
    lastMouseX = position.getX();
    lastMouseY = position.getY();

    // if the mouse was clicked
    if (jme.mouseWasClicked())
    {
        if (jme.mods.isMiddleButtonDown())
        {
            handleMiddleButterDown(jme);
        }
        if (jme.mods.isLeftButtonDown())
        {
            handleLeftButtonDown(jme);
        }
    }
}

void ArrangementArea::handleMiddleButterDown(const juce::MouseEvent &jme)
{
    // handle resize/mode mode activation
    if (activityManager.getAppState().getUiState() == UI_STATE_DEFAULT)
    {
        activityManager.getAppState().setUiState(UI_STATE_VIEW_RESIZING);
    }
}

void ArrangementArea::handleLeftButtonDown(const juce::MouseEvent &jme)
{
    int clickedTrack;
    // handle click when in default mode
    if (activityManager.getAppState().getUiState() == UI_STATE_DEFAULT)
    {
        juce::Optional<SampleBorder> sampleBorderClicked = mouseOverSelectionBorder();
        // if we're clicking around a cursor
        if (mouseOverPlayCursor())
        {
            // enter cursor moving mode
            activityManager.getAppState().setUiState(UI_STATE_CURSOR_MOVING);
            // else, see if we're clicking tracks for selection
        }
        else if (sampleBorderClicked.hasValue())
        {

            switch (sampleBorderClicked->border)
            {
            case BORDER_LEFT:
                activityManager.getAppState().setUiState(UI_STATE_MOUSE_DRAG_SAMPLE_START);
                dragLastPosition = lastMouseX;
                break;
            case BORDER_LOWER:
                if (sampleBorderClicked->direction == LOW_FREQS_TO_BOTTOM)
                {
                    activityManager.getAppState().setUiState(UI_STATE_MOUSE_DRAG_MONO_HIGHPASS);
                }
                else
                {
                    activityManager.getAppState().setUiState(UI_STATE_MOUSE_DRAG_MONO_LOWPASS);
                }
                dragLastPosition = lastMouseY;
                break;
            case BORDER_UPPER:
                if (sampleBorderClicked->direction == LOW_FREQS_TO_BOTTOM)
                {
                    activityManager.getAppState().setUiState(UI_STATE_MOUSE_DRAG_MONO_LOWPASS);
                }
                else
                {
                    activityManager.getAppState().setUiState(UI_STATE_MOUSE_DRAG_MONO_HIGHPASS);
                }
                dragLastPosition = lastMouseY;
                break;
            case BORDER_RIGHT:
                activityManager.getAppState().setUiState(UI_STATE_MOUSE_DRAG_SAMPLE_LENGTH);
                dragLastPosition = lastMouseX;
                break;
            }
        }
        else
        {
            clickedTrack = getTrackClicked();
            if (clickedTrack != -1)
            {
                // if ctrl is not pressed, we clear selection set
                if (!jme.mods.isCtrlDown())
                {
                    selectedTracks.clear();
                }
                selectedTracks.insert(clickedTrack);
                repaint();
                // if clicking in the void, unselected everything
            }
            else
            {
                if (!selectedTracks.empty())
                {
                    selectedTracks.clear();
                    repaint();
                }
            }
        }
    }
}

/**
 * Give id of track clicked (label or sample fft).
 * @return     Index of the sample clicked in the tracks/samples arrays. -1 if
 * no match.
 */
int ArrangementArea::getTrackClicked()
{
    size_t nTracks = samples.size();
    int64_t trackPosition;

    int bestTrackIndex = -1;
    float bestTrackIntensity = 0.0f;
    float currentIntensity;

    // prioritize selection through label clicking
    for (size_t i = 0; i < onScreenLabelsPixelsCoords.size(); i++)
    {
        if (onScreenLabelsPixelsCoords[i].contains(lastMouseX, lastMouseY))
        {
            return onScreenLabelsPixelsCoords[i].getSampleIndex();
        }
    }

    // for each track
    for (size_t i = 0; i < nTracks; i++)
    {
        // skip nullptr in tracks list (should not happen)
        if (samples[i]->isDisabled())
        {
            continue;
        }

        trackPosition = (samples[i]->getFramePosition() - viewPosition) / viewScale;

        // if it's inbound, return the index
        if (lastMouseX > trackPosition && lastMouseX < trackPosition + (samples[i]->getFrameLength() / viewScale))
        {
            float x = float(lastMouseX - trackPosition) / float(samples[i]->getFrameLength() / viewScale);

            float y = float(lastMouseY) / bounds.getHeight();

            currentIntensity = samples[i]->textureIntensity(x, y);

            if (bestTrackIndex == -1 || currentIntensity > bestTrackIntensity)
            {
                bestTrackIndex = i;
                bestTrackIntensity = currentIntensity;
            }
        }
    }

    // if there was a significant intensity for this track/sample, return its id
    if (bestTrackIntensity > FREQVIEW_MIN_SAMPLE_CLICK_INTENSITY)
    {
        return bestTrackIndex;
    }
    else
    {
        return -1;
    }
}

void ArrangementArea::initSelectedTracksDrag()
{
    std::set<size_t>::iterator itr;
    for (itr = selectedTracks.begin(); itr != selectedTracks.end(); itr++)
    {
        samples[*itr]->initDrag();
    }
}

void ArrangementArea::updateSelectedTracksDrag(int pixelShift)
{
    std::set<size_t>::iterator itr;
    for (itr = selectedTracks.begin(); itr != selectedTracks.end(); itr++)
    {
        openGLContext.executeOnGLThread(
            [this, itr, pixelShift](juce::OpenGLContext &c) { samples[*itr]->updateDrag(pixelShift * viewScale); },
            true);
    }
}

void ArrangementArea::mouseUp(const juce::MouseEvent &jme)
{
    juce::Point<int> position = jme.getPosition();
    // saving last mouse position
    lastMouseX = position.getX();
    lastMouseY = position.getY();

    // it's a bit counter intuitive, when isXButtonDown is true,
    // it means that our even was about leaving this state

    if (jme.mods.isLeftButtonDown())
    {
        handleLeftButtonUp(jme);
    }

    if (activityManager.getAppState().getUiState() == UI_STATE_VIEW_RESIZING && jme.mods.isMiddleButtonDown())
    {
        activityManager.getAppState().setUiState(UI_STATE_DEFAULT);
    }
}

void ArrangementArea::handleLeftButtonUp(const juce::MouseEvent &jme)
{
    switch (activityManager.getAppState().getUiState())
    {

    case UI_STATE_CURSOR_MOVING:
        activityManager.getAppState().setUiState(UI_STATE_DEFAULT);
        mixingBus.setNextReadPosition(viewPosition + lastMouseX * viewScale);
        break;

    case UI_STATE_MOUSE_DRAG_SAMPLE_START:
        activityManager.getAppState().setUiState(UI_STATE_DEFAULT);
        break;

    case UI_STATE_MOUSE_DRAG_SAMPLE_LENGTH:
        activityManager.getAppState().setUiState(UI_STATE_DEFAULT);
        break;

    case UI_STATE_MOUSE_DRAG_MONO_LOWPASS:
    case UI_STATE_MOUSE_DRAG_MONO_HIGHPASS:
        activityManager.getAppState().setUiState(UI_STATE_DEFAULT);
        break;

    case UI_STATE_DEFAULT:
    case UI_STATE_VIEW_RESIZING:
    case UI_STATE_KEYBOARD_SAMPLE_DRAG:
        break;
    }
}

void ArrangementArea::mouseDrag(const juce::MouseEvent &jme)
{
    juce::Point<int> newPosition = jme.getPosition();
    bool viewUpdated = false;

    switch (activityManager.getAppState().getUiState())
    {

    case UI_STATE_VIEW_RESIZING:
        viewUpdated = updateViewResizing(newPosition);
        break;
    case UI_STATE_MOUSE_DRAG_SAMPLE_START:
        cropSampleEdgeHorizontally(true);
        break;

    case UI_STATE_MOUSE_DRAG_SAMPLE_LENGTH:
        cropSampleEdgeHorizontally(false);
        break;

    case UI_STATE_MOUSE_DRAG_MONO_LOWPASS:
        cropSampleBordersVertically(false);
        break;

    case UI_STATE_MOUSE_DRAG_MONO_HIGHPASS:
        cropSampleBordersVertically(true);
        break;

    case UI_STATE_DEFAULT:
    case UI_STATE_CURSOR_MOVING:
    case UI_STATE_KEYBOARD_SAMPLE_DRAG:
        break;
    }

    // saving last mouse position
    lastMouseX = newPosition.getX();
    lastMouseY = newPosition.getY();

    // if updated view or in cursor moving mode, repaint
    if (viewUpdated || activityManager.getAppState().getUiState() == UI_STATE_CURSOR_MOVING)
    {
        updateShadersPositionUniforms();
        repaint();
    }
}

void ArrangementArea::cropSampleBordersVertically(bool innerBorders)
{
    // compute the frequency to set in the filter
    float filterFreq = verticalPositionToFrequency(lastMouseY);
    // set the filter frequency for each sample
    std::set<size_t>::iterator itr;

    SamplePlayer *currentSample;

    bool changedSomething = false;

    for (itr = selectedTracks.begin(); itr != selectedTracks.end(); itr++)
    {
        currentSample = mixingBus.getTrack(*itr);
        if (currentSample != nullptr)
        {
            changedSomething = true;
            if (innerBorders)
            {
                currentSample->setHighPassFreq(filterFreq);
            }
            else
            {
                currentSample->setLowPassFreq(filterFreq);
            }
            // apply change to opengl sample
            openGLContext.executeOnGLThread(
                [this, itr, currentSample](juce::OpenGLContext &c) {
                    samples[*itr]->updatePropertiesAndUploadToGpu(currentSample);
                },
                true);
        }
    }

    if (changedSomething)
    {
        repaint();
    }
}

float ArrangementArea::verticalPositionToFrequency(int y)
{
    // REMINDER: upper half (below half height) is the first
    // fft with lower frequencies below. second halve freqs are the opposite disposition.

    // freqRatio is the ratio from 0 to max frequency (AUDIO_FRAMRATE).
    // It's not linear to freqs, we therefore need to invert our index correction
    // from texture freq index to storage freq index and then from storage
    // freq index to fft index.
    float freqRatio = 0.0f;
    if (y < (FREQTIME_VIEW_HEIGHT >> 1))
    {
        freqRatio = 1.0f - (float(y) / float(FREQTIME_VIEW_HEIGHT >> 1));
    }
    else
    {
        freqRatio = (float(y) / float(FREQTIME_VIEW_HEIGHT >> 1)) - 1.0f;
    }

    // apply back the index transformation to make it linear to frequencies
    float textureFreqIndex = freqRatio * float(FREQVIEW_SAMPLE_FFT_SCOPE_SIZE - 1);
    float storageFreqIndex = UnitConverter::magnifyTextureFrequencyIndexInv(textureFreqIndex);
    float fftFreqIndex = UnitConverter::magnifyFftIndex(storageFreqIndex);
    // map the fft index to a frequency
    float freq = fftFreqIndex * (AUDIO_FRAMERATE / FREQVIEW_SAMPLE_FFT_SIZE);
    return juce::jlimit(0.0f, float(AUDIO_FRAMERATE >> 1), freq);
}

bool ArrangementArea::updateViewResizing(juce::Point<int> &newPosition)
{
    // ratio from horizontal to vertical movement
    float movementRatio = ((float)abs(lastMouseX - newPosition.getX())) / ((float)abs(lastMouseY - newPosition.getY()));

    int pixelMovement, frameMovement;
    // store previous value to later deduce if we updated
    int64_t oldViewScale, oldViewPosition;
    oldViewPosition = viewPosition;
    oldViewScale = viewScale;

    // if ratio from horizontal to vertical is higher than thresold
    if (movementRatio > FREQVIEW_MIN_HORIZONTAL_MOVE_RATIO)
    {
        // move for horizontal position
        pixelMovement = newPosition.getX() - lastMouseX;
        frameMovement = pixelMovement * viewScale;
        // avoid having the position going below zero
        if (frameMovement > 0 && frameMovement <= viewPosition)
        {
            viewPosition -= frameMovement;
        }
        else if (frameMovement < 0)
        {
            viewPosition -= frameMovement;
        }
    }

    // if ratio from horizontal to vertical movement is smaller than half
    if (movementRatio < FREQVIEW_MIN_VERTICAL_MOVE_RATIO)
    {
        // scale for vertical movements
        pixelMovement = newPosition.getY() - lastMouseY;
        // cap the pixel movement to avoid harsh movements
        if (abs(pixelMovement) > FREQVIEW_MAX_ABSOLUTE_SCALE_MOVEMENT)
        {
            // this weird instruction is getting the sign of the integer fast
            pixelMovement = ((pixelMovement > 0) - (pixelMovement < 0)) * FREQVIEW_MAX_ABSOLUTE_SCALE_MOVEMENT;
        }
        viewScale = viewScale + pixelMovement;
        // check if within bounds
        if (viewScale < FREQVIEW_MIN_SCALE_FRAME_PER_PIXEL)
        {
            viewScale = FREQVIEW_MIN_SCALE_FRAME_PER_PIXEL;
        }
        if (viewScale > FREQVIEW_MAX_SCALE_FRAME_PER_PIXEL)
        {
            viewScale = FREQVIEW_MAX_SCALE_FRAME_PER_PIXEL;
        }
    }
    // save some repait by comparing viewScale and viewPosition
    return (oldViewPosition != viewPosition || oldViewScale != viewScale);
}

void ArrangementArea::mouseMove(const juce::MouseEvent &jme)
{
    // saving last mouse position
    juce::Point<int> newPosition = jme.getPosition();
    lastMouseX = newPosition.getX();
    lastMouseY = newPosition.getY();

    switch (activityManager.getAppState().getUiState())
    {
    case UI_STATE_KEYBOARD_SAMPLE_DRAG:
        updateSelectedTracksDrag(lastMouseX - trackMovingInitialPosition);
        repaint();
        break;
    case UI_STATE_MOUSE_DRAG_MONO_LOWPASS:
    case UI_STATE_MOUSE_DRAG_MONO_HIGHPASS:
    case UI_STATE_MOUSE_DRAG_SAMPLE_START:
    case UI_STATE_CURSOR_MOVING:
    case UI_STATE_VIEW_RESIZING:
    case UI_STATE_MOUSE_DRAG_SAMPLE_LENGTH:
    case UI_STATE_DEFAULT:
        break;
    }

    updateMouseCursor();
}

void ArrangementArea::cropSampleEdgeHorizontally(bool cropFront)
{
    // compute distange to beginning
    int distanceInFrames = (lastMouseX - dragLastPosition) * viewScale;
    if (abs(distanceInFrames) < FREQVIEW_MIN_RESIZE_FRAMES)
    {
        return;
    }
    dragLastPosition = lastMouseX;
    // current track to be edited
    SamplePlayer *currentTrack;
    // for each track in the selection
    std::set<size_t>::iterator itr;
    int actualFrameChange;

    for (itr = selectedTracks.begin(); itr != selectedTracks.end(); itr++)
    {
        currentTrack = mixingBus.getTrack(*itr);
        // if possible to dra
        if (currentTrack != nullptr)
        {

            if (cropFront)
            {
                actualFrameChange = currentTrack->tryMovingStart(distanceInFrames);
            }
            else
            {
                actualFrameChange = currentTrack->tryMovingEnd(distanceInFrames);
            }

            if (actualFrameChange != 0)
            {
                // apply change to opengl sample
                openGLContext.executeOnGLThread(
                    [this, itr, currentTrack](juce::OpenGLContext &c) {
                        samples[*itr]->updatePropertiesAndUploadToGpu(currentTrack);
                    },
                    true);
            }
        }
    }
    repaint();
}

void ArrangementArea::updateMouseCursor()
{
    if (activityManager.getAppState().getUiState() == UI_STATE_DEFAULT)
    {
        if (getMouseCursor() == juce::MouseCursor::NormalCursor)
        {
            juce::Optional<SampleBorder> borderUnderMouse = mouseOverSelectionBorder();

            if (mouseOverPlayCursor())
            {
                setMouseCursor(juce::MouseCursor::DraggingHandCursor);
            }
            else if (borderUnderMouse.hasValue())
            {
                switch (borderUnderMouse->border)
                {
                case BORDER_LEFT:
                    setMouseCursor(juce::MouseCursor::LeftEdgeResizeCursor);
                    break;

                case BORDER_RIGHT:
                    setMouseCursor(juce::MouseCursor::RightEdgeResizeCursor);
                    break;

                case BORDER_UPPER:
                    setMouseCursor(juce::MouseCursor::TopEdgeResizeCursor);
                    break;

                case BORDER_LOWER:
                    setMouseCursor(juce::MouseCursor::BottomEdgeResizeCursor);
                    break;
                }
            }
        }
        else if (getMouseCursor() == juce::MouseCursor::DraggingHandCursor)
        {
            if (!mouseOverPlayCursor())
            {
                setMouseCursor(juce::MouseCursor::NormalCursor);
            }
        }
        else if (getMouseCursor() == juce::MouseCursor::LeftEdgeResizeCursor ||
                 getMouseCursor() == juce::MouseCursor::RightEdgeResizeCursor ||
                 getMouseCursor() == juce::MouseCursor::TopEdgeResizeCursor ||
                 getMouseCursor() == juce::MouseCursor::BottomEdgeResizeCursor)
        {
            juce::Optional<SampleBorder> borderUnderMouse = mouseOverSelectionBorder();
            if (!borderUnderMouse.hasValue())
            {
                setMouseCursor(juce::MouseCursor::NormalCursor);
            }
        }
    }
}

bool ArrangementArea::mouseOverPlayCursor()
{
    if (abs(lastMouseX - lastPlayCursorPosition) < PLAYCURSOR_GRAB_WIDTH)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool ArrangementArea::keyPressed(const juce::KeyPress &key)
{
    // if the space key is pressed, play or pause
    if (key == juce::KeyPress::spaceKey)
    {
        if (mixingBus.isCursorPlaying())
        {
            mixingBus.stopPlayback();
        }
        else if (activityManager.getAppState().getUiState() != UI_STATE_CURSOR_MOVING)
        {
            mixingBus.startPlayback();
        }
    }
    else if (key == juce::KeyPress::createFromDescription(KEYMAP_DRAG_MODE) ||
             key == juce::KeyPress::createFromDescription(std::string("ctrl + ") + KEYMAP_DRAG_MODE))
    {
        int newPosition;
        // if pressing d and not in any mode, start dragging
        if (activityManager.getAppState().getUiState() == UI_STATE_DEFAULT)
        {
            // if ctrl is pressed, duplicate and add to selection
            if (key.getModifiers().isCtrlDown())
            {
                // if nothing is selected, abort
                if (selectedTracks.size() == 0)
                {
                    return false;
                }

                // get lowest track position is selection
                int64_t selectionBeginPos = lowestStartPosInSelection();

                // iterate over selected tracks to duplicate everything
                std::set<std::size_t>::iterator it = selectedTracks.begin();

                while (it != selectedTracks.end())
                {
                    // insert selected tracks at the mouse cursor position
                    int pos = mixingBus.getTrack(*it)->getEditingPosition();
                    newPosition = (viewPosition + (lastMouseX * viewScale)) + (pos - selectionBeginPos);
                    SampleImportTask task(newPosition, *it);
                    mixingBus.addSample(task);
                    it++;
                }
                recentlyDuplicated = true;
            }
            else
            {
                // start dragging
                trackMovingInitialPosition = lastMouseX;
                activityManager.getAppState().setUiState(UI_STATE_KEYBOARD_SAMPLE_DRAG);
                initSelectedTracksDrag();
            }
        }
    }
    else if (key == juce::KeyPress::createFromDescription(KEYMAP_DELETE_SELECTION))
    {
        // if pressing x and not in any mode, delete selected tracks
        if (activityManager.getAppState().getUiState() == UI_STATE_DEFAULT && !selectedTracks.empty())
        {
            deleteSelectedTracks();
        }
    }
    // do not intercept the signal and pass it around
    return false;
}

void ArrangementArea::syncSampleColor(int sampleIndex)
{
    openGLContext.executeOnGLThread(
        [this, sampleIndex](juce::OpenGLContext &c) {
            samples[sampleIndex]->setColor(taxonomyManager.getSampleColor(sampleIndex));
        },
        true);
}

// lowestStartPosInSelection will get the lowest track
// position in the selected tracks. It returns 0
// if no track is selected.
int64_t ArrangementArea::lowestStartPosInSelection()
{
    int lowestTrackPos = 0;
    int initialized = false;
    std::set<std::size_t>::iterator it = selectedTracks.begin();
    while (it != selectedTracks.end())
    {
        int pos = mixingBus.getTrack(*it)->getEditingPosition();
        if (initialized == false || pos < lowestTrackPos)
        {
            initialized = true;
            lowestTrackPos = pos;
        }
        it++;
    }
    return lowestTrackPos;
}

void ArrangementArea::deleteSelectedTracks()
{
    // for each selected track
    std::set<std::size_t>::iterator it = selectedTracks.begin();
    while (it != selectedTracks.end())
    {
        // delete it
        SamplePlayer *deletedSp = mixingBus.deleteTrack(*it);
        delete deletedSp;
        it++;
    }
    // clear selection and redraw
    selectedTracks.clear();
    repaint();
}

bool ArrangementArea::keyStateChanged(bool isKeyDown)
{
    // if in drag mode
    if (activityManager.getAppState().getUiState() == UI_STATE_KEYBOARD_SAMPLE_DRAG)
    {
        // if the D key is not pressed anymore
        if (!juce::KeyPress::isKeyCurrentlyDown(juce::KeyPress::createFromDescription(KEYMAP_DRAG_MODE).getKeyCode()))
        {
            // update tracks position, get out of drag mode, and repaint
            size_t nTracks = mixingBus.getNumTracks();
            SamplePlayer *sp;
            int64_t trackPosition;
            int64_t dragDistance = (lastMouseX - trackMovingInitialPosition) * viewScale;
            // TODO: iterate over set rather than looping over all tracks
            // for each track
            for (size_t i = 0; i < nTracks; i++)
            {
                if (auto search = selectedTracks.find(i); search != selectedTracks.end())
                {
                    // get a reference to the sample
                    sp = mixingBus.getTrack(i);
                    // skip nullptr in tracks list (should not happen)
                    if (sp == nullptr)
                    {
                        continue;
                    }
                    // get its lock
                    const juce::SpinLock::ScopedLockType lock(sp->playerMutex);
                    // get the old position
                    trackPosition = sp->getEditingPosition();
                    trackPosition += dragDistance;
                    sp->move(trackPosition);
                    openGLContext.executeOnGLThread(
                        [this, sp, i](juce::OpenGLContext &c) { this->samples[i]->updatePropertiesAndUploadToGpu(sp); },
                        true);
                }
            }
            activityManager.getAppState().setUiState(UI_STATE_DEFAULT);
            repaint();
        }
    }
    if (recentlyDuplicated &&
        !juce::KeyPress::isKeyCurrentlyDown(juce::KeyPress::createFromDescription(KEYMAP_DRAG_MODE).getKeyCode()) &&
        !juce::KeyPress::isKeyCurrentlyDown(
            juce::KeyPress::createFromDescription(std::string("ctrl + ") + KEYMAP_DRAG_MODE).getKeyCode()))
    {
        recentlyDuplicated = false;
    }

    return false;
}

bool ArrangementArea::isInterestedInDragSource(const SourceDetails &dragSourceDetails)
{
    juce::String filename = dragSourceDetails.description.toString();
    if (filename.startsWith("file:"))
    {
        if (filename.endsWith(".mp3") || filename.endsWith(".wav"))
        {
            return true;
        }
    }
    return false;
}

bool ArrangementArea::isInterestedInFileDrag(const juce::StringArray &files)
{
    // are the files sound files that are supported
    return mixingBus.filePathsValid(files);
}

void ArrangementArea::filesDropped(const juce::StringArray &files, int x, int y)
{
    // converts x to an valid position in audio frame
    int64_t framePos = viewPosition + (x * viewScale);
    // we try to load the samples
    for (int i = 0; i < files.size(); i++)
    {
        SampleImportTask task(files[i].toStdString(), framePos);
        mixingBus.addSample(task);
    }
}

void ArrangementArea::itemDropped(const SourceDetails &dragSourceDetails)
{
    int x = dragSourceDetails.localPosition.getX();
    // converts x to an valid position in audio frame
    int64_t framePos = viewPosition + (x * viewScale);
    // we try to load the sample
    juce::String filename = dragSourceDetails.description.toString().replaceFirstOccurrenceOf("file:", "");
    SampleImportTask task(filename.toStdString(), framePos);
    mixingBus.addSample(task);
}