#include "ArrangementArea.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#include "../../Arrangement/ColorPalette.h"
#include "../../Arrangement/NumericInputId.h"
#include "../../OpenGL/FreqviewShaders.h"
#include "../../OpenGL/GLInfoLogger.h"
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
ArrangementArea::ArrangementArea(MixingBus &mb, ActivityManager &am)
    : activityManager(am), taxonomyManager(am.getAppState().getTaxonomy()), tempoGrid(am), mixingBus(mb)
{
    activityManager.registerTaskListener(this);

    resetArrangement();

    // broadcast tempo value as a task
    std::shared_ptr<NumericInputUpdateTask> tempoUpdate = std::make_shared<NumericInputUpdateTask>(NUM_INPUT_ID_TEMPO);
    tempoUpdate->newValue = tempo;
    tempoUpdate->setCompleted(true);
    activityManager.broadcastTask(tempoUpdate);

    // play cursor color
    cursorColor = juce::Colour(240, 240, 240);
    // enable keyboard events
    setWantsKeyboardFocus(true);

    shadersCompiled = false;

    // Indicates that no part of this Component is transparent.
    setOpaque(true);

    openGLContext.setRenderer(this);
    openGLContext.attachTo(*this);

    textureManager->setOpenGlContext(&openGLContext);

    addAndMakeVisible(frequencyGrid);
    addAndMakeVisible(tempoGrid);
}

void ArrangementArea::resetArrangement()
{
    // save reference to the sample manager
    // initialize grid and position
    viewPosition = 0;
    viewScale = 100;
    lastMouseX = 0;
    lastMouseY = 0;
    lastPlayCursorPosition = 0;
    trackMovingInitialPosition = -1;

    tempoGrid.updateView(viewPosition, viewScale);

    tempo = DEFAULT_TEMPO;
    tempoGrid.updateTempo(DEFAULT_TEMPO);

    selectedTracks.clear();
    copyAndBroadcastSelection(true);

    samples.clear();
}

ArrangementArea::~ArrangementArea()
{
    openGLContext.detach();
    openGLContext.setRenderer(nullptr);
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
    paintSplitLocation(g);
}

void ArrangementArea::paintOverChildren(juce::Graphics &g)
{
    paintLabels(g);
    paintSelectionArea(g);
    paintPlayCursor(g);
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

    cropSelectedSamplesPosition.swap(selectedSamplesCoordsBuffer);
}

bool ArrangementArea::taskHandler(std::shared_ptr<Task> task)
{
    std::shared_ptr<SampleDisplayTask> sc = std::dynamic_pointer_cast<SampleDisplayTask>(task);
    if (sc != nullptr && !sc->isCompleted() && !sc->hasFailed())
    {
        createNewSampleMeshAndTaxonomy(sc->sample, sc->creationTask);
        return true;
    }

    std::shared_ptr<SampleDeletionDisplayTask> disableTask = std::dynamic_pointer_cast<SampleDeletionDisplayTask>(task);
    if (disableTask != nullptr && !disableTask->isCompleted() && !disableTask->hasFailed())
    {
        samples[(size_t)disableTask->id]->disable();

        if (selectedTracks.find((size_t)disableTask->id) != selectedTracks.end())
        {
            selectedTracks.erase((size_t)disableTask->id);
            copyAndBroadcastSelection(true);
        }

        disableTask->setCompleted(true);
        disableTask->setFailed(false);
        return true;
    }

    std::shared_ptr<SampleRestoreDisplayTask> restoreTask = std::dynamic_pointer_cast<SampleRestoreDisplayTask>(task);
    if (restoreTask != nullptr && !restoreTask->isCompleted() && !restoreTask->hasFailed())
    {
        samples[(size_t)restoreTask->id].reset();

        samples[(size_t)restoreTask->id] = std::make_shared<SampleGraphicModel>(
            restoreTask->restoredSample, taxonomyManager.getSampleColor(restoreTask->id));

        // when a sample openGL model is created it doesn't allocate the GPU
        // data unless we ask for it.
        openGLContext.executeOnGLThread(
            [this, restoreTask](juce::OpenGLContext &) { samples[(size_t)restoreTask->id]->registerGlObjects(); },
            true);

        // we will repaint to display the sample again
        repaint();

        return true;
    }

    std::shared_ptr<SampleUpdateTask> updateTask = std::dynamic_pointer_cast<SampleUpdateTask>(task);
    if (updateTask != nullptr && !updateTask->isCompleted() && !updateTask->hasFailed())
    {
        openGLContext.executeOnGLThread(
            [this, updateTask](juce::OpenGLContext &) {
                samples[(size_t)updateTask->id]->reloadSampleData(updateTask->sample);
            },
            true);

        updateTask->setCompleted(true);
        updateTask->setFailed(false);

        return true;
    }

    std::shared_ptr<SampleGroupRecolor> colorTask = std::dynamic_pointer_cast<SampleGroupRecolor>(task);
    if (colorTask != nullptr && !colorTask->isCompleted() && !colorTask->hasFailed())
    {

        recolorSelection(colorTask);

        colorTask->setCompleted(true);
        colorTask->setFailed(false);
        return true;
    }

    std::shared_ptr<NumericInputUpdateTask> numFieldUpdateTask =
        std::dynamic_pointer_cast<NumericInputUpdateTask>(task);
    if (numFieldUpdateTask != nullptr && !numFieldUpdateTask->isCompleted())
    {
        // if it's a task to apply a tempo update, do it, mark completed, and broadcast change
        if (numFieldUpdateTask->numericalInputId == NUM_INPUT_ID_TEMPO)
        {

            // if it's just a broadcasting request, simply broadcast existing value
            if (numFieldUpdateTask->isBroadcastRequest)
            {
                numFieldUpdateTask->newValue = tempo;
                numFieldUpdateTask->setCompleted(true);
                activityManager.broadcastNestedTaskNow(numFieldUpdateTask);
                return true;
            }

            // if this task wants to set a new value, well set it if possible
            if (numFieldUpdateTask->newValue <= MAX_TEMPO && numFieldUpdateTask->newValue >= MIN_TEMPO)
            {
                // +0.5f to round instead of truncating
                tempo = int(numFieldUpdateTask->newValue + 0.5f);

                tempoGrid.updateTempo(tempo);

                // this one has the side effect of uploading new tempo grid widths for the background shader
                shaderUniformUpdateThreadWrapper();

                numFieldUpdateTask->setCompleted(true);
                activityManager.broadcastNestedTaskNow(numFieldUpdateTask);

                repaint();

                return true;
            }
        }
    }

    auto fadeChange = std::dynamic_pointer_cast<SampleFadeChange>(task);
    if (fadeChange != nullptr && fadeChange->isCompleted() && !fadeChange->hasFailed() &&
        mixingBus.getTrack(fadeChange->sampleId) != nullptr)
    {
        openGLContext.executeOnGLThread(
            [this, fadeChange](juce::OpenGLContext &) {
                samples[(size_t)fadeChange->sampleId]->reloadSampleData(mixingBus.getTrack(fadeChange->sampleId));
            },
            true);

        return false;
    }

    auto filterRepChange = std::dynamic_pointer_cast<SampleFilterRepeatChange>(task);
    if (filterRepChange != nullptr && filterRepChange->isCompleted() && !filterRepChange->hasFailed() &&
        mixingBus.getTrack(filterRepChange->sampleId) != nullptr)
    {
        openGLContext.executeOnGLThread(
            [this, filterRepChange](juce::OpenGLContext &) {
                samples[(size_t)filterRepChange->sampleId]->reloadSampleData(
                    mixingBus.getTrack(filterRepChange->sampleId));
            },
            true);

        return false;
    }

    auto resetTask = std::dynamic_pointer_cast<ResetTask>(task);
    if (resetTask != nullptr)
    {
        resetArrangement();

        // broadcast tempo value as a task
        std::shared_ptr<NumericInputUpdateTask> tempoUpdate =
            std::make_shared<NumericInputUpdateTask>(NUM_INPUT_ID_TEMPO);
        tempoUpdate->newValue = tempo;
        tempoUpdate->setCompleted(true);
        activityManager.broadcastNestedTaskNow(tempoUpdate);

        taxonomyManager.reset();

        shaderUniformUpdateThreadWrapper(false);

        repaint();

        resetTask->markStepDoneAndCheckCompletion();

        return false;
    }

    auto loadProjectTask = std::dynamic_pointer_cast<OpenProjectTask>(task);
    if (loadProjectTask != nullptr && loadProjectTask->stage == OPEN_PROJECT_STAGE_ARRANGEMENT_SETUP &&
        !loadProjectTask->hasFailed())
    {
        try
        {
            unmarshal(loadProjectTask->uiConfig);

            loadProjectTask->stage = OPEN_PROJECT_STAGE_MIXBUS_SETUP;
            activityManager.broadcastNestedTaskNow(loadProjectTask);
        }
        catch (std::exception &err)
        {
            loadProjectTask->setFailed(true);
            loadProjectTask->stage = OPEN_PROJECT_STAGE_FAILED;

            std::string errMsg = std::string() + "unable to open project on arrangement area side: " + err.what();

            std::cerr << errMsg << std::endl;

            auto notifTask = std::make_shared<NotificationTask>("Unable to open project. See logs for more infos.",
                                                                ERROR_NOTIF_TYPE);
            activityManager.broadcastNestedTaskNow(notifTask);
        }

        return true;
    }

    return false;
}

void ArrangementArea::paintSplitLocation(juce::Graphics &g)
{
    // the split location is only displayed in these ui states
    if (activityManager.getAppState().getUiState() != UI_STATE_DISPLAY_FREQUENCY_SPLIT_LOCATION &&
        activityManager.getAppState().getUiState() != UI_STATE_DISPLAY_TIME_SPLIT_LOCATION)
    {
        return;
    }

    g.setColour(COLOR_SPLIT_PLACEHOLDER);

    // iterate over selected items
    std::set<size_t>::iterator itr;
    for (itr = selectedTracks.begin(); itr != selectedTracks.end(); itr++)
    {
        auto samplesRects = samples[*itr]->getPixelBounds(viewPosition, viewScale, bounds.getHeight());

        for (size_t i = 0; i < samplesRects.size(); i++)
        {
            // reject what's not on screen
            if (samplesRects[i].getX() + samplesRects[i].getWidth() < 0 || samplesRects[i].getX() > bounds.getWidth())
            {
                continue;
            }

            // reject what's not on the line and draw the line
            if (activityManager.getAppState().getUiState() == UI_STATE_DISPLAY_FREQUENCY_SPLIT_LOCATION)
            {
                if (samplesRects[i].getY() > lastMouseY ||
                    samplesRects[i].getY() + samplesRects[i].getHeight() < lastMouseY)
                {
                    continue;
                }

                g.drawHorizontalLine(lastMouseY, samplesRects[i].getX(),
                                     samplesRects[i].getX() + samplesRects[i].getWidth());
                g.drawHorizontalLine(bounds.getHeight() - lastMouseY, samplesRects[i].getX(),
                                     samplesRects[i].getX() + samplesRects[i].getWidth());
            }
            if (activityManager.getAppState().getUiState() == UI_STATE_DISPLAY_TIME_SPLIT_LOCATION)
            {
                if (samplesRects[i].getX() > lastMouseX ||
                    samplesRects[i].getX() + samplesRects[i].getWidth() < lastMouseX)
                {
                    continue;
                }
                g.drawVerticalLine(lastMouseX, samplesRects[i].getY(),
                                   samplesRects[i].getY() + samplesRects[i].getHeight());
            }
        }
    }
}

void ArrangementArea::paintSelectionArea(juce::Graphics &g)
{
    if (activityManager.getAppState().getUiState() == UI_STATE_SELECT_AREA_WITH_MOUSE)
    {
        g.setColour(COLOR_SELECT_AREA.withAlpha(0.2f));
        g.fillRoundedRectangle(currentSelectionRect, FREQVIEW_LABELS_CORNER_ROUNDING);

        g.setColour(COLOR_SELECT_AREA);
        g.drawRoundedRectangle(currentSelectionRect, 4, 1);
    }
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
    for (size_t i = 0; i < cropSelectedSamplesPosition.size(); i++)
    {
        // if our click is in bounds of this sample
        if (cropSelectedSamplesPosition[i].expanded(PLAYCURSOR_GRAB_WIDTH).contains(lastMouseX, lastMouseY))
        {

            SampleDirection direction = LOW_FREQS_TO_TOP;
            if (cropSelectedSamplesPosition[i].getPartId() == 0)
            {
                direction = LOW_FREQS_TO_BOTTOM;
            }

            // switch on the border
            if (abs(cropSelectedSamplesPosition[i].getX() - lastMouseX) < PLAYCURSOR_GRAB_WIDTH)
            {
                return juce::Optional<SampleBorder>(
                    SampleBorder(cropSelectedSamplesPosition[i].getSampleIndex(), BORDER_LEFT, direction));
            }

            if (abs(cropSelectedSamplesPosition[i].getX() + cropSelectedSamplesPosition[i].getWidth() - lastMouseX) <
                PLAYCURSOR_GRAB_WIDTH)
            {
                return juce::Optional<SampleBorder>(
                    SampleBorder(cropSelectedSamplesPosition[i].getSampleIndex(), BORDER_RIGHT, direction));
            }

            if (abs(cropSelectedSamplesPosition[i].getY() - lastMouseY) < PLAYCURSOR_GRAB_WIDTH)
            {
                return juce::Optional<SampleBorder>(
                    SampleBorder(cropSelectedSamplesPosition[i].getSampleIndex(), BORDER_UPPER, direction));
            }

            if (abs(cropSelectedSamplesPosition[i].getY() + cropSelectedSamplesPosition[i].getHeight() - lastMouseY) <
                PLAYCURSOR_GRAB_WIDTH)
            {
                return juce::Optional<SampleBorder>(
                    SampleBorder(cropSelectedSamplesPosition[i].getSampleIndex(), BORDER_LOWER, direction));
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

    for (size_t i = 0; i < samples.size(); i++)
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
    for (size_t i = 0; i < rectangles.size(); i++)
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
    auto sampleRects = samples[(size_t)sampleIndex]->getPixelBounds(viewPosition, viewScale, bounds.getHeight());

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
    bool selected = selectedTracks.find((size_t)index) != selectedTracks.end();

    // different border width depending on if selected or not
    if (selected)
    {
        g.setColour(juce::Colour::fromFloatRGBA(0.0f, 0.0f, 0.0f, 0.75f));
        g.fillRoundedRectangle(box, FREQVIEW_LABELS_CORNER_ROUNDING);
        g.setColour(COLOR_LABELS_BORDER);
        g.drawRoundedRectangle(box, FREQVIEW_LABELS_CORNER_ROUNDING, FREQVIEW_LABELS_BORDER_THICKNESS);
    }
    else
    {
        g.setColour(juce::Colour::fromFloatRGBA(0.0f, 0.0f, 0.0f, 0.55f));
        g.fillRoundedRectangle(box, FREQVIEW_LABELS_CORNER_ROUNDING);
        g.setColour(COLOR_LABELS_BORDER.withAlpha(0.8f));
        g.drawRoundedRectangle(box, FREQVIEW_LABELS_CORNER_ROUNDING, FREQVIEW_LABELS_BORDER_THICKNESS / 2.0f);
    }

    auto reducedBox = box.reduced(FREQVIEW_LABELS_MARGINS, FREQVIEW_LABELS_MARGINS);

    g.setColour(taxonomyManager.getSampleColor(index));
    g.fillRect(reducedBox.withWidth(box.getHeight() - FREQVIEW_LABELS_MARGINS));

    if (selected)
    {
        g.setColour(COLOR_LABELS_BORDER);
    }
    else
    {
        g.setColour(COLOR_LABELS_BORDER.withAlpha(0.8f));
    }

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
        shaderUniformUpdateThreadWrapper();
    }

    tempoGrid.setBounds(getLocalBounds());
    frequencyGrid.setBounds(getLocalBounds());
}

void ArrangementArea::newOpenGLContextCreated()
{
    std::cerr << "Initializing OpenGL context..." << std::endl;
    // Instanciate an instance of OpenGLShaderProgram
    texturedPositionedShader.reset(new juce::OpenGLShaderProgram(openGLContext));
    backgroundGridShader.reset(new juce::OpenGLShaderProgram(openGLContext));
    // Compile and link the shader
    if (buildAllShaders())
    {
        shadersCompiled = true;

        std::cerr << "Sucessfully compiled OpenGL shaders" << std::endl;

        texturedPositionedShader->use();
        texturedPositionedShader->setUniform("ourTexture", 0);
        texturedPositionedShader->setUniform("alphaMask", 1);

        shaderUniformUpdateThreadWrapper(true);

        // log some info about openGL version and all
        logOpenGLInfoCallback(openGLContext);

        // enable the error logging
        enableOpenGLErrorLogging();

        // load the alpha mask texture into the main shader
        alphaMaskTextureLoader.loadTexture();

        // initialize background openGL objects
        backgroundGrid.registerGlObjects();
    }
    else
    {
        std::cerr << "FATAL: Unable to compile OpenGL Shaders" << std::endl;
        juce::JUCEApplicationBase::quit();
    }
}

bool ArrangementArea::buildAllShaders()
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

void ArrangementArea::shaderUniformUpdateThreadWrapper(bool fromGlThread)
{
    // send the new view positions to opengl thread
    if (!fromGlThread)
    {
        openGLContext.executeOnGLThread([this](juce::OpenGLContext &) { updateShadersViewAndGridUniforms(); }, false);
    }
    else
    {
        updateShadersViewAndGridUniforms();
    }
}

void ArrangementArea::updateShadersViewAndGridUniforms()
{
    texturedPositionedShader->use();
    texturedPositionedShader->setUniform("viewPosition", (GLfloat)viewPosition);
    texturedPositionedShader->setUniform("viewWidth", (GLfloat)(bounds.getWidth() * viewScale));

    computeShadersGridUniformsVars();

    backgroundGridShader->use();
    backgroundGridShader->setUniform("grid0PixelShift", (GLint)grid0PixelShift);
    backgroundGridShader->setUniform("grid0PixelWidth", (GLfloat)grid0PixelWidth);

    backgroundGridShader->setUniform("grid1PixelShift", (GLint)grid1PixelShift);
    backgroundGridShader->setUniform("grid1PixelWidth", (GLfloat)grid1PixelWidth);

    backgroundGridShader->setUniform("grid2PixelShift", (GLint)grid2PixelShift);
    backgroundGridShader->setUniform("grid2PixelWidth", (GLfloat)grid2PixelWidth);

    backgroundGridShader->setUniform("viewHeightPixels", (GLfloat)(bounds.getHeight()));
}

void ArrangementArea::computeShadersGridUniformsVars()
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

    alphaMaskTextureLoader.bindTexture();
    texturedPositionedShader->use();

    for (size_t i = 0; i < samples.size(); i++)
    {
        samples[i]->drawGlObjects();
    }
}

void ArrangementArea::openGLContextClosing()
{
}

void ArrangementArea::createNewSampleMeshAndTaxonomy(std::shared_ptr<SamplePlayer> sp,
                                                     std::shared_ptr<SampleCreateTask> task)
{
    // create graphic objects from the sample
    if (task->reuseNewId)
    {
        samples[(size_t)task->newIndex]->reenable();
    }
    else
    {
        std::shared_ptr<SampleGraphicModel> sampleRef =
            std::make_shared<SampleGraphicModel>(sp, taxonomyManager.getSampleColor(task->getAllocatedIndex()));
        samples.push_back(sampleRef);

        // fatal error if sample ids are different
        if ((samples.size() - 1) != (size_t)task->getAllocatedIndex())
        {
            std::cerr << "FATAL: Sample ids don't match in Mixbus and ArrangementArea" << std::endl;
            juce::JUCEApplicationBase::quit();
            return;
        }
    }

    if (sp != nullptr)
    {

        // assign default name to sample
        taxonomyManager.setSampleName(task->getAllocatedIndex(), sp->getFileName());
        // send the data to the GPUs from the OpenGL thread
        openGLContext.executeOnGLThread(
            [this, task](juce::OpenGLContext &) { samples[(size_t)task->newIndex]->registerGlObjects(); }, true);
        // if it's a copy, set the group and update the color
        if (task->isDuplication())
        {
            taxonomyManager.copyTaxonomy(task->getDuplicateTargetId(), task->getAllocatedIndex());
            setSampleColorFromTaxonomy(task->getAllocatedIndex());

            // refresh properties of the track we just splitted (it changed size of filters)
            if (task->getDuplicationType() == DUPLICATION_TYPE_SPLIT_AT_FREQUENCY ||
                task->getDuplicationType() == DUPLICATION_TYPE_SPLIT_AT_POSITION)
            {
                refreshSampleOpenGlView(task->getDuplicateTargetId());
            }

            selectedTracks.insert((size_t)task->getAllocatedIndex());
        }
    }

    task->setCompleted(true);
    task->setFailed(false);
}

void ArrangementArea::paintPlayCursor(juce::Graphics &g)
{
    g.setColour(cursorColor);
    // in the cursor moving phase, we avoid waiting tracks locks
    // by using the mouse value
    if (activityManager.getAppState().getUiState() != UI_STATE_CURSOR_MOVING)
    {
        lastPlayCursorPosition = ((mixingBus.getNextReadPosition() - viewPosition) / viewScale);
        g.fillRect(lastPlayCursorPosition - (PLAYCURSOR_WIDTH >> 1), 0, PLAYCURSOR_WIDTH, getBounds().getHeight());
    }
    else
    {
        lastPlayCursorPosition = lastMouseX;
        g.fillRect(lastPlayCursorPosition - (PLAYCURSOR_WIDTH >> 1), 0, PLAYCURSOR_WIDTH, getBounds().getHeight());
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

void ArrangementArea::handleMiddleButterDown(const juce::MouseEvent &)
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
                saveInitialSelectionGainRamps();
                dragLastPosition = lastMouseX;
                dragDistanceMap.clear();
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
                initFiltersFreqs.clear();
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
                initFiltersFreqs.clear();
                break;

            case BORDER_RIGHT:
                activityManager.getAppState().setUiState(UI_STATE_MOUSE_DRAG_SAMPLE_LENGTH);
                saveInitialSelectionGainRamps();
                dragLastPosition = lastMouseX;
                dragDistanceMap.clear();
                break;
            }
        }
        else
        {
            clickedTrack = getSampleIdUnderCursor();
            if (clickedTrack != -1)
            {
                // if ctrl is not pressed, we clear selection set
                if (!jme.mods.isCtrlDown())
                {
                    selectedTracks.clear();
                    selectedTracks.insert((size_t)clickedTrack);
                }
                else
                {
                    if (selectedTracks.find((size_t)clickedTrack) == selectedTracks.end())
                    {
                        selectedTracks.insert((size_t)clickedTrack);
                    }
                    else
                    {
                        selectedTracks.erase((size_t)clickedTrack);
                    }
                }

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
            copyAndBroadcastSelection(false);
        }
    }

    if (activityManager.getAppState().getUiState() == UI_STATE_DISPLAY_FREQUENCY_SPLIT_LOCATION ||
        activityManager.getAppState().getUiState() == UI_STATE_DISPLAY_TIME_SPLIT_LOCATION)
    {
        // iterate over selected tracks to duplicate everything
        std::set<std::size_t>::iterator it = selectedTracks.begin();

        int groupId = Task::getNewTaskGroupIndex();

        while (it != selectedTracks.end())
        {
            if (activityManager.getAppState().getUiState() == UI_STATE_DISPLAY_FREQUENCY_SPLIT_LOCATION)
            {
                float freq = UnitConverter::verticalPositionToFrequency(lastMouseY, getBounds().getHeight());
                // create a track duplicate from the sample id at *it
                std::shared_ptr<SampleCreateTask> task = std::make_shared<SampleCreateTask>(freq, *it);
                task->setTaskGroupIndex(groupId);
                activityManager.broadcastTask(task);
            }
            else
            {
                auto xSampleLocations = samples[*it]->getPixelBounds(viewPosition, viewScale, bounds.getHeight());
                if (lastMouseX > xSampleLocations[0].getX() &&
                    lastMouseX < xSampleLocations[0].getX() + xSampleLocations[0].getWidth())
                {
                    int frameSplitPosition = (lastMouseX - xSampleLocations[0].getX()) * viewScale;
                    std::shared_ptr<SampleCreateTask> task =
                        std::make_shared<SampleCreateTask>(frameSplitPosition, *it, DUPLICATION_TYPE_SPLIT_AT_POSITION);
                    task->setTaskGroupIndex(groupId);
                    activityManager.broadcastTask(task);
                }
            }
            it++;
        }

        activityManager.getAppState().setUiState(UI_STATE_DEFAULT);
    }
}

void ArrangementArea::saveInitialSelectionGainRamps()
{
    selectedSamplesFrameGainRamps.clear();
    std::set<size_t>::iterator itr;
    for (itr = selectedTracks.begin(); itr != selectedTracks.end(); itr++)
    {
        if (*itr >= mixingBus.getNumTracks() || mixingBus.getTrack(*itr) == nullptr)
        {
            continue;
        }

        selectedSamplesFrameGainRamps[*itr] = std::pair<int, int>(mixingBus.getTrack(*itr)->getFadeInLength(),
                                                                  mixingBus.getTrack(*itr)->getFadeOutLength());
    }
}

int ArrangementArea::getSampleIdUnderCursor()
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
        // skip nullptr in tracks list
        if (samples[i]->isDisabled())
        {
            continue;
        }

        trackPosition = (samples[i]->getFramePosition() - viewPosition) / viewScale;

        // if it's inbound, return the index
        if (lastMouseX > trackPosition && lastMouseX < trackPosition + (samples[i]->getFrameLength() / viewScale))
        {
            float x = float(lastMouseX - trackPosition) / (float(samples[i]->getFrameLength()) / float(viewScale));

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

float ArrangementArea::getTextureIntensityUnderCursor()
{
    size_t nTracks = samples.size();
    int64_t trackPosition;

    int bestTrackIndex = -1;
    float bestTrackIntensity = 0.0f;
    float currentIntensity;

    // for each track
    for (size_t i = 0; i < nTracks; i++)
    {
        // skip nullptr in tracks list
        if (samples[i]->isDisabled())
        {
            continue;
        }

        trackPosition = (samples[i]->getFramePosition() - viewPosition) / viewScale;

        // if it's inbound, return the index
        if (lastMouseX > trackPosition && lastMouseX < trackPosition + (samples[i]->getFrameLength() / viewScale))
        {
            float x = float(lastMouseX - trackPosition) / (float(samples[i]->getFrameLength()) / float(viewScale));

            float y = float(lastMouseY) / bounds.getHeight();

            currentIntensity = samples[i]->textureIntensity(x, y);

            if (bestTrackIndex == -1 || currentIntensity > bestTrackIntensity)
            {
                bestTrackIndex = i;
                bestTrackIntensity = currentIntensity;
            }
        }
    }

    return bestTrackIntensity;
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
            [this, itr, pixelShift](juce::OpenGLContext &) { samples[*itr]->updateDrag(pixelShift * viewScale); },
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

void ArrangementArea::handleLeftButtonUp(const juce::MouseEvent &)
{

    switch (activityManager.getAppState().getUiState())
    {

    case UI_STATE_CURSOR_MOVING:
        activityManager.getAppState().setUiState(UI_STATE_DEFAULT);
        mixingBus.setNextReadPosition(viewPosition + lastMouseX * viewScale);
        break;

    case UI_STATE_MOUSE_DRAG_SAMPLE_START:
        activityManager.getAppState().setUiState(UI_STATE_DEFAULT);
        emitTimeDragTasks(true);
        break;

    case UI_STATE_MOUSE_DRAG_SAMPLE_LENGTH:
        activityManager.getAppState().setUiState(UI_STATE_DEFAULT);
        emitTimeDragTasks(false);
        break;

    case UI_STATE_MOUSE_DRAG_MONO_LOWPASS:
        activityManager.getAppState().setUiState(UI_STATE_DEFAULT);
        emitFilterDragTasks(true);
        break;

    case UI_STATE_MOUSE_DRAG_MONO_HIGHPASS:
        activityManager.getAppState().setUiState(UI_STATE_DEFAULT);
        emitFilterDragTasks(false);
        break;

    case UI_STATE_SELECT_AREA_WITH_MOUSE:
        copyAndBroadcastSelection(false);
        activityManager.getAppState().setUiState(UI_STATE_DEFAULT);
        repaint();
        break;

    case UI_STATE_DEFAULT:
    case UI_STATE_VIEW_RESIZING:
    case UI_STATE_KEYBOARD_SAMPLE_DRAG:
    case UI_STATE_DISPLAY_FREQUENCY_SPLIT_LOCATION:
    case UI_STATE_DISPLAY_TIME_SPLIT_LOCATION:
        break;
    case UI_STATE_MOUSE_DRAG_NUMERIC_INPUT:
    case UI_STATE_RESIZE_MAINVIEW:
    case UI_STATE_MOVE_LOOP_SECTION:
        break;
    }
}

void ArrangementArea::emitTimeDragTasks(bool cropBeginning)
{
    // iterate over map distances and for each push a completed
    // task to task listeners (to store it in history)
    std::map<int, int>::iterator it;
    int taskGroupId = Task::getNewTaskGroupIndex();
    for (it = dragDistanceMap.begin(); it != dragDistanceMap.end(); it++)
    {
        if (it->first < 0 || (size_t)it->first >= mixingBus.getNumTracks() || mixingBus.getTrack(it->first) == nullptr)
        {
            continue;
        }

        std::shared_ptr<SampleTimeCropTask> task =
            std::make_shared<SampleTimeCropTask>(cropBeginning, it->first, it->second);

        if (selectedSamplesFrameGainRamps.find(it->first) != selectedSamplesFrameGainRamps.end())
        {
            auto initialGainRamps = selectedSamplesFrameGainRamps[it->first];
            task->initialFadeInFrameLen = initialGainRamps.first;
            task->initialFadeOutFrameLen = initialGainRamps.second;
        }
        task->finalFadeInFrameLen = mixingBus.getTrack(it->first)->getFadeInLength();
        task->finalFadeOutFrameLen = mixingBus.getTrack(it->first)->getFadeOutLength();

        task->setTaskGroupIndex(taskGroupId);
        task->setCompleted(true);
        activityManager.broadcastTask(task);
    }
}

void ArrangementArea::emitFilterDragTasks(bool isLowPass)
{
    // iterate over map distances and for each push a completed
    // task to task listeners (to store it in history)
    std::map<int, float>::iterator it;
    int taskGroupId = Task::getNewTaskGroupIndex();
    for (it = initFiltersFreqs.begin(); it != initFiltersFreqs.end(); it++)
    {
        float finalFreq;
        if (isLowPass)
        {
            finalFreq = mixingBus.getTrack(it->first)->getLowPassFreq();
        }
        else
        {
            finalFreq = mixingBus.getTrack(it->first)->getHighPassFreq();
        }

        std::shared_ptr<SampleFreqCropTask> task =
            std::make_shared<SampleFreqCropTask>(isLowPass, it->first, it->second, finalFreq);
        task->setTaskGroupIndex(taskGroupId);
        task->setCompleted(true);
        activityManager.broadcastTask(task);
    }
}

void ArrangementArea::mouseDrag(const juce::MouseEvent &jme)
{
    juce::Point<int> newPosition = jme.getPosition();
    bool viewUpdated = false;

    emitPositionTip();

    switch (activityManager.getAppState().getUiState())
    {

    case UI_STATE_VIEW_RESIZING:
        viewUpdated = updateViewResizing(newPosition);
        break;
    case UI_STATE_MOUSE_DRAG_SAMPLE_START:
        cropSelectedSamplesPos(true);
        break;

    case UI_STATE_MOUSE_DRAG_SAMPLE_LENGTH:
        cropSelectedSamplesPos(false);
        break;

    case UI_STATE_MOUSE_DRAG_MONO_LOWPASS:
        cropSelectedSamplesFreqs(false);
        break;

    case UI_STATE_MOUSE_DRAG_MONO_HIGHPASS:
        cropSelectedSamplesFreqs(true);
        break;

    case UI_STATE_DEFAULT:
        if (jme.mods.isCtrlDown() && jme.mods.isLeftButtonDown())
        {
            activityManager.getAppState().setUiState(UI_STATE_SELECT_AREA_WITH_MOUSE);
            startSelectX = newPosition.getX();
            startSelectY = newPosition.getY();
            addToSelectionFromSelectionArea();
        }
        break;

    case UI_STATE_SELECT_AREA_WITH_MOUSE:
        addToSelectionFromSelectionArea();
        repaint();
        break;

    case UI_STATE_CURSOR_MOVING:
    case UI_STATE_KEYBOARD_SAMPLE_DRAG:
    case UI_STATE_DISPLAY_FREQUENCY_SPLIT_LOCATION:
    case UI_STATE_DISPLAY_TIME_SPLIT_LOCATION:
    case UI_STATE_MOUSE_DRAG_NUMERIC_INPUT:
    case UI_STATE_RESIZE_MAINVIEW:
    case UI_STATE_MOVE_LOOP_SECTION:
        break;
    }

    // saving last mouse position
    lastMouseX = newPosition.getX();
    lastMouseY = newPosition.getY();

    // if updated view or in cursor moving mode, repaint
    if (viewUpdated || activityManager.getAppState().getUiState() == UI_STATE_CURSOR_MOVING)
    {
        shaderUniformUpdateThreadWrapper();
        repaint();
    }
}

void ArrangementArea::addToSelectionFromSelectionArea()
{
    currentSelectionRect = juce::Rectangle<float>(juce::Point<float>(startSelectX, startSelectY),
                                                  juce::Point<float>(lastMouseX, lastMouseY));

    for (size_t i = 0; i < samples.size(); i++)
    {
        if (samples[i] == nullptr || samples[i]->isDisabled())
        {
            continue;
        }

        auto sampleAreas = samples[i]->getPixelBounds(viewPosition, viewScale, bounds.getHeight());
        for (size_t j = 0; j < sampleAreas.size(); j++)
        {
            if (currentSelectionRect.intersects(sampleAreas[j]))
            {
                if (selectedTracks.find(i) == selectedTracks.end())
                {
                    selectedTracks.insert(i);
                }
            }
        }
    }
}

void ArrangementArea::mouseWheelMove(const juce::MouseEvent &e, const juce::MouseWheelDetails &wheel)
{
    if (activityManager.getAppState().getUiState() == UI_STATE_DEFAULT)
    {
        if (!wheel.isInertial && std::abs(wheel.deltaY) > 0.001)
        {

            if (e.mods.isShiftDown())
            {
                viewPosition = viewPosition - ((float)(viewScale * 200) * wheel.deltaY);
                if (viewPosition < 0)
                {
                    viewPosition = 0;
                }
            }
            else
            {

                viewScale = (float)viewScale * (1.0f - wheel.deltaY);

                if (viewScale < FREQVIEW_MIN_SCALE_FRAME_PER_PIXEL)
                {
                    viewScale = FREQVIEW_MIN_SCALE_FRAME_PER_PIXEL;
                }
                if (viewScale > FREQVIEW_MAX_SCALE_FRAME_PER_PIXEL)
                {
                    viewScale = FREQVIEW_MAX_SCALE_FRAME_PER_PIXEL;
                }
            }

            tempoGrid.updateView(viewPosition, viewScale);
            repaint();
            shaderUniformUpdateThreadWrapper(false);
        }
    }
}

void ArrangementArea::cropSelectedSamplesFreqs(bool innerBorders)
{
    // compute the frequency to set in the filter
    float filterFreq = UnitConverter::verticalPositionToFrequency(lastMouseY, getBounds().getHeight());
    // set the filter frequency for each sample
    std::set<size_t>::iterator itr;

    std::shared_ptr<SamplePlayer> currentSample;

    bool changedSomething = false;

    for (itr = selectedTracks.begin(); itr != selectedTracks.end(); itr++)
    {
        currentSample = mixingBus.getTrack(*itr);
        if (currentSample != nullptr)
        {

            // if it's the first time it's being moved,
            // record its initial freq
            if (initFiltersFreqs.find(*itr) == initFiltersFreqs.end())
            {
                float freq;
                if (innerBorders)
                {
                    freq = currentSample->getHighPassFreq();
                }
                else
                {
                    freq = currentSample->getLowPassFreq();
                }

                initFiltersFreqs[*itr] = freq;
            }

            changedSomething = true;
            if (innerBorders)
            {
                currentSample->setHighPassFreq(filterFreq);
            }
            else
            {
                currentSample->setLowPassFreq(filterFreq);
            }
            refreshSampleOpenGlView(*itr);
        }
    }

    if (changedSomething)
    {
        repaint();
    }
}

void ArrangementArea::refreshSampleOpenGlView(int index)
{
    std::shared_ptr<SamplePlayer> sp = mixingBus.getTrack(index);
    if (sp == nullptr)
    {
        return;
    }
    openGLContext.executeOnGLThread(
        [this, index, sp](juce::OpenGLContext &) { samples[(size_t)index]->reloadSampleData(sp); }, true);
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

    tempoGrid.updateView(viewPosition, viewScale);

    // save some repait by comparing viewScale and viewPosition
    return (oldViewPosition != viewPosition || oldViewScale != viewScale);
}

void ArrangementArea::emitPositionTip()
{
    float currentFreq = UnitConverter::verticalPositionToFrequency(lastMouseY, getBounds().getHeight());
    std::string posTip = "" + std::to_string(int(currentFreq + 0.5f)) + " Hz";

    float secs = (viewPosition + (lastMouseX * viewScale)) / float(AUDIO_FRAMERATE);
    float ms = (secs - std::floor(secs)) * 1000;
    posTip += "     |    " + std::to_string(int(secs)) +
              " s"
              "   " +
              std::to_string(int(ms + 0.5f)) + " ms";

    posTip += "     |    " + std::to_string(int(viewScale)) + " samples/pixel";

    int sampleUnderMouse = getSampleIdUnderCursor();
    if (sampleUnderMouse != -1)
    {
        std::string name = taxonomyManager.getSampleName(sampleUnderMouse);
        posTip += "     |    " + name;
    }

    float intensity = getTextureIntensityUnderCursor();
    float intensityDB = UnitConverter::magnifyIntensityInv(intensity);
    posTip += "     |    " + std::to_string(intensityDB) + " dB";

    sharedTips->setPositionStatus(posTip);
}

void ArrangementArea::mouseExit(const juce::MouseEvent &)
{
    sharedTips->setPositionStatus("");
}

void ArrangementArea::mouseMove(const juce::MouseEvent &jme)
{
    // saving last mouse position
    juce::Point<int> newPosition = jme.getPosition();
    lastMouseX = newPosition.getX();
    lastMouseY = newPosition.getY();

    emitPositionTip();

    switch (activityManager.getAppState().getUiState())
    {

    // Note: Ugly real life optimization, if UI_STATE_KEYBOARD_SAMPLE_DRAG is
    // hit the whole blocks executes up the next break !!
    case UI_STATE_KEYBOARD_SAMPLE_DRAG:
        updateSelectedTracksDrag(lastMouseX - trackMovingInitialPosition);
    case UI_STATE_DISPLAY_FREQUENCY_SPLIT_LOCATION:
    case UI_STATE_DISPLAY_TIME_SPLIT_LOCATION:
        repaint();
        break;

    case UI_STATE_MOUSE_DRAG_MONO_LOWPASS:
    case UI_STATE_MOUSE_DRAG_MONO_HIGHPASS:
    case UI_STATE_MOUSE_DRAG_SAMPLE_START:
    case UI_STATE_CURSOR_MOVING:
    case UI_STATE_VIEW_RESIZING:
    case UI_STATE_MOUSE_DRAG_SAMPLE_LENGTH:
    case UI_STATE_SELECT_AREA_WITH_MOUSE:
    case UI_STATE_DEFAULT:
    case UI_STATE_MOUSE_DRAG_NUMERIC_INPUT:
    case UI_STATE_RESIZE_MAINVIEW:
    case UI_STATE_MOVE_LOOP_SECTION:
        break;
    }

    updateMouseCursor();
}

void ArrangementArea::cropSelectedSamplesPos(bool cropFront)
{
    // compute distange to beginning
    int distanceInFrames = (lastMouseX - dragLastPosition) * viewScale;
    if (abs(distanceInFrames) < FREQVIEW_MIN_RESIZE_FRAMES)
    {
        return;
    }
    dragLastPosition = lastMouseX;
    // current track to be edited
    std::shared_ptr<SamplePlayer> currentTrack;
    // for each track in the selection
    std::set<size_t>::iterator itr;
    int actualFrameChange;

    for (itr = selectedTracks.begin(); itr != selectedTracks.end(); itr++)
    {
        currentTrack = mixingBus.getTrack(*itr);
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

                if (dragDistanceMap.find(*itr) != dragDistanceMap.end())
                {
                    dragDistanceMap[*itr] = dragDistanceMap[*itr] + actualFrameChange;
                }
                else
                {
                    dragDistanceMap[*itr] = actualFrameChange;
                }

                refreshSampleOpenGlView(*itr);
            }
        }
    }
    repaint();
}

void ArrangementArea::updateMouseCursor()
{
    if (activityManager.getAppState().getUiState() == UI_STATE_DEFAULT)
    {
        juce::Optional<SampleBorder> borderUnderMouse = mouseOverSelectionBorder();

        if (mouseOverPlayCursor())
        {
            setMouseCursor(juce::MouseCursor::DraggingHandCursor);
            return;
        }
        else if (borderUnderMouse.hasValue())
        {
            switch (borderUnderMouse->border)
            {
            case BORDER_LEFT:
                setMouseCursor(juce::MouseCursor::LeftEdgeResizeCursor);
                return;

            case BORDER_RIGHT:
                setMouseCursor(juce::MouseCursor::RightEdgeResizeCursor);
                return;

            case BORDER_UPPER:
                setMouseCursor(juce::MouseCursor::TopEdgeResizeCursor);
                return;

            case BORDER_LOWER:
                setMouseCursor(juce::MouseCursor::BottomEdgeResizeCursor);
                return;
            }
        }

        setMouseCursor(juce::MouseCursor::NormalCursor);
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
            auto task = std::make_shared<PlayStateUpdateTask>(false, false);
            activityManager.broadcastTask(task);
        }
        else if (activityManager.getAppState().getUiState() != UI_STATE_CURSOR_MOVING)
        {
            auto task = std::make_shared<PlayStateUpdateTask>(true, false);
            activityManager.broadcastTask(task);
        }
    }
    else if (key == juce::KeyPress::createFromDescription(KEYMAP_DRAG_MODE) ||
             key == juce::KeyPress::createFromDescription(std::string("ctrl + ") + KEYMAP_DRAG_MODE))
    {
        int newPosition;
        // if pressing d and not in any mode, start dragging
        if (activityManager.getAppState().getUiState() == UI_STATE_DEFAULT)
        {
            // if ctrl is pressed, the user duplicates selected samples
            // and add them to current selection
            if (key.getModifiers().isCtrlDown())
            {
                // if nothing is selected, abort
                if (selectedTracks.size() == 0)
                {
                    return false;
                }

                // get lowest track position is selection
                int64_t selectionBeginPos = lowestStartPosInSelection();

                // use a copy of track selection to prevent iterating over newly added items
                std::set<std::size_t> selectedTrackAtClick = selectedTracks;

                int groupTaskId = Task::getNewTaskGroupIndex();

                // iterate over selected tracks to duplicate everything
                std::set<std::size_t>::iterator it = selectedTrackAtClick.begin();

                while (it != selectedTrackAtClick.end())
                {
                    // insert selected tracks at the mouse cursor position
                    int pos = mixingBus.getTrack(*it)->getEditingPosition();
                    newPosition = (viewPosition + (lastMouseX * viewScale)) + (pos - selectionBeginPos);
                    std::shared_ptr<SampleCreateTask> task =
                        std::make_shared<SampleCreateTask>(newPosition, *it, DUPLICATION_TYPE_COPY_AT_POSITION);
                    task->setTaskGroupIndex(groupTaskId);
                    activityManager.broadcastTask(task);
                    it++;
                }
            }
            else // if ctrl is not pressed the user starts moving selected samples around
            {
                // start dragging
                trackMovingInitialPosition = lastMouseX;
                activityManager.getAppState().setUiState(UI_STATE_KEYBOARD_SAMPLE_DRAG);
                initSelectedTracksDrag();
            }
        }
    }
    else if (key == juce::KeyPress::createFromDescription(KEYMAP_DELETE_SELECTION) ||
             key == juce::KeyPress::createFromDescription("delete"))
    {
        // if pressing x and not in any mode, delete selected tracks
        if (activityManager.getAppState().getUiState() == UI_STATE_DEFAULT && !selectedTracks.empty())
        {
            deleteSelectedTracks();
        }
    }
    else if (key == juce::KeyPress::createFromDescription(KEYMAP_SPLIT_SAMPLE_AT_FREQUENCY))
    {
        if (activityManager.getAppState().getUiState() == UI_STATE_DEFAULT && !selectedTracks.empty())
        {
            activityManager.getAppState().setUiState(UI_STATE_DISPLAY_FREQUENCY_SPLIT_LOCATION);
        }
    }
    else if (key == juce::KeyPress::createFromDescription(KEYMAP_SPLIT_SAMPLE_AT_TIME))
    {
        if (activityManager.getAppState().getUiState() == UI_STATE_DEFAULT && !selectedTracks.empty())
        {
            activityManager.getAppState().setUiState(UI_STATE_DISPLAY_TIME_SPLIT_LOCATION);
        }
    }
    else if (key == juce::KeyPress::createFromDescription(KEYMAP_UNDO))
    {
        if (activityManager.getAppState().getUiState() == UI_STATE_DEFAULT)
        {
            activityManager.undoLastActivity();
            repaint();
        }
    }
    else if (key == juce::KeyPress::createFromDescription(KEYMAP_REDO))
    {
        if (activityManager.getAppState().getUiState() == UI_STATE_DEFAULT)
        {
            activityManager.redoLastActivity();
            repaint();
        }
    }
    // do not intercept the signal and pass it around
    return false;
}

void ArrangementArea::setSampleColorFromTaxonomy(int sampleIndex)
{
    openGLContext.executeOnGLThread(
        [this, sampleIndex](juce::OpenGLContext &) {
            samples[(size_t)sampleIndex]->setColor(taxonomyManager.getSampleColor(sampleIndex));
        },
        true);
}

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
    // tasks to broadcast
    std::vector<std::shared_ptr<Task>> tasksToBroadcast;

    int taskGroupId = Task::getNewTaskGroupIndex();

    auto tracksToDelete = selectedTracks;

    // first clear and broadcast selection.
    // this preventing slowdown on large group deletion
    // where deletion tasks repeateadly update selection
    // and get selected sample inputs widget and mixbus
    // to do tons of updates.
    selectedTracks.clear();
    copyAndBroadcastSelection(false);

    // for each selected track
    std::set<size_t>::iterator it;
    for (it = tracksToDelete.begin(); it != tracksToDelete.end(); it++)
    {
        if (mixingBus.getTrack(*it) != nullptr)
        {
            std::shared_ptr<SampleDeletionTask> delTask = std::make_shared<SampleDeletionTask>(*it);
            delTask->setTaskGroupIndex(taskGroupId);
            tasksToBroadcast.push_back(delTask);
        }
    }

    // separetely broadcast to prevent altering selection set during iteration
    for (size_t i = 0; i < tasksToBroadcast.size(); i++)
    {
        activityManager.broadcastTask(tasksToBroadcast[i]);
    }

    repaint();
}

bool ArrangementArea::keyStateChanged(bool)
{
    // if in drag mode
    if (activityManager.getAppState().getUiState() == UI_STATE_KEYBOARD_SAMPLE_DRAG)
    {
        // if the D key is not pressed anymore
        if (!juce::KeyPress::isKeyCurrentlyDown(juce::KeyPress::createFromDescription(KEYMAP_DRAG_MODE).getKeyCode()))
        {

            int taskGroupId = Task::getNewTaskGroupIndex();

            // update tracks position, get out of drag mode, and repaint
            std::shared_ptr<SamplePlayer> sp;
            int64_t trackPosition;
            int64_t dragDistance = (lastMouseX - trackMovingInitialPosition) * viewScale;

            std::set<size_t>::iterator it;
            for (it = selectedTracks.begin(); it != selectedTracks.end(); it++)
            {
                // get a reference to the sample
                sp = mixingBus.getTrack(*it);
                // skip nullptr in tracks list (should not happen)
                if (sp == nullptr)
                {
                    continue;
                }

                {
                    // get its lock
                    const juce::SpinLock::ScopedLockType lock(sp->playerMutex);

                    // get the old position and apply it
                    trackPosition = sp->getEditingPosition();
                    trackPosition += dragDistance;
                    sp->move(trackPosition);
                }

                // refresh the openGL data
                refreshSampleOpenGlView(*it);

                // broadcast a completed task so that it's recorded in history
                std::shared_ptr<SampleMovingTask> task = std::make_shared<SampleMovingTask>(*it, dragDistance);
                task->setTaskGroupIndex(taskGroupId);
                task->setCompleted(true);
                activityManager.broadcastTask(task);
            }

            activityManager.getAppState().setUiState(UI_STATE_DEFAULT);
            repaint();
        }
    }

    else if (activityManager.getAppState().getUiState() == UI_STATE_DISPLAY_FREQUENCY_SPLIT_LOCATION)
    {
        if (!juce::KeyPress::isKeyCurrentlyDown(
                juce::KeyPress::createFromDescription(KEYMAP_SPLIT_SAMPLE_AT_FREQUENCY).getKeyCode()))
        {
            activityManager.getAppState().setUiState(UI_STATE_DEFAULT);
            repaint();
        }
    }

    else if (activityManager.getAppState().getUiState() == UI_STATE_DISPLAY_TIME_SPLIT_LOCATION)
    {
        if (!juce::KeyPress::isKeyCurrentlyDown(
                juce::KeyPress::createFromDescription(KEYMAP_SPLIT_SAMPLE_AT_TIME).getKeyCode()))
        {
            activityManager.getAppState().setUiState(UI_STATE_DEFAULT);
            repaint();
        }
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

void ArrangementArea::filesDropped(const juce::StringArray &files, int x, int)
{
    // converts x to an valid position in audio frame
    int64_t framePos = viewPosition + (x * viewScale);
    // we try to load the samples
    for (int i = 0; i < files.size(); i++)
    {
        std::shared_ptr<SampleCreateTask> task = std::make_shared<SampleCreateTask>(files[i].toStdString(), framePos);
        activityManager.broadcastTask(task);
    }
}

void ArrangementArea::itemDropped(const SourceDetails &dragSourceDetails)
{
    int x = dragSourceDetails.localPosition.getX();
    // converts x to an valid position in audio frame
    int64_t framePos = viewPosition + (x * viewScale);
    // we try to load the sample
    juce::String filename = dragSourceDetails.description.toString().replaceFirstOccurrenceOf("file:", "");

    std::shared_ptr<SampleCreateTask> task = std::make_shared<SampleCreateTask>(filename.toStdString(), framePos);
    activityManager.broadcastTask(task);
}

void ArrangementArea::recolorSelection(std::shared_ptr<SampleGroupRecolor> task)
{
    std::set<int> samplesToRefreshDIsplay;
    if (task->colorFromId)
    {
        // not really vital, but we'll prevent duplicate color setting
        // with this set
        std::set<int> changedGroups;
        std::set<size_t>::iterator it;

        std::set<int> changedIds;

        std::map<int, juce::Colour> oldSampleColours;

        // always copying the selection is a good habit
        std::set<size_t> currentSelection = selectedTracks;
        for (it = currentSelection.begin(); it != currentSelection.end(); it++)
        {
            auto initialColor = taxonomyManager.getSampleColor(*it);

            samplesToRefreshDIsplay.insert(*it);
            changedIds.insert(*it);
            oldSampleColours[*it] = initialColor;

            int gid = taxonomyManager.getSampleGroup(*it);
            if (changedGroups.find(gid) == changedGroups.end())
            {
                changedGroups.insert(gid);
                taxonomyManager.setGroupColor(gid, colourPalette[(size_t)task->colorId]);

                // now for each group we add its samples to the changed set
                std::set<int> groupSamples = taxonomyManager.getGroupSamples(gid);
                std::set<int>::iterator itGroupSamples;
                for (itGroupSamples = groupSamples.begin(); itGroupSamples != groupSamples.end(); itGroupSamples++)
                {
                    samplesToRefreshDIsplay.insert(*itGroupSamples);
                    changedIds.insert(*itGroupSamples);
                    oldSampleColours[*itGroupSamples] = initialColor;
                }
            }
        }

        task->newColor = colourPalette[(size_t)task->colorId];
        task->changedSampleIds = changedIds;
        task->colorsPerId = oldSampleColours;
    }
    else
    {
        std::set<int> changedGroups;
        std::set<int>::iterator it;
        for (it = task->changedSampleIds.begin(); it != task->changedSampleIds.end(); it++)
        {
            samplesToRefreshDIsplay.insert(*it);
            int gid = taxonomyManager.getSampleGroup(*it);
            if (changedGroups.find(gid) == changedGroups.end())
            {
                changedGroups.insert(gid);
                taxonomyManager.setGroupColor(gid, task->colorsPerId[*it]);
            }
        }
    }

    // now resync all the colours and we're done
    std::set<int>::iterator it;
    for (it = samplesToRefreshDIsplay.begin(); it != samplesToRefreshDIsplay.end(); it++)
    {
        setSampleColorFromTaxonomy(*it);
    }
    repaint();
}

void ArrangementArea::copyAndBroadcastSelection(bool fromWithinTask)
{
    // create task with a copy of the selection set
    std::shared_ptr<SelectionChangingTask> selectionUpdate = std::make_shared<SelectionChangingTask>(selectedTracks);
    selectionUpdate->setCompleted(true);
    selectionUpdate->setFailed(false);

    if (fromWithinTask)
    {
        activityManager.broadcastNestedTaskNow(selectionUpdate);
    }
    else
    {
        activityManager.broadcastTask(selectionUpdate);
    }
}

std::string ArrangementArea::marshal()
{
    json output = {{"view_position", viewPosition}, {"view_scale", viewScale}};
    return output.dump(JSON_STATE_SAVING_INDENTATION);
}

void ArrangementArea::unmarshal(std::string &s)
{
    json input = json::parse(s);
    input.at("view_position").get_to(viewPosition);
    input.at("view_scale").get_to(viewScale);

    tempoGrid.updateView(viewPosition, viewScale);

    shaderUniformUpdateThreadWrapper(false);

    repaint();
}