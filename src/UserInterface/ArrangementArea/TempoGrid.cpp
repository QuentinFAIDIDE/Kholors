#include "TempoGrid.h"

#include "../../Config.h"
#include <memory>

#define BAR_LINE_HEIGHT 8
#define SUBBAR_LINE_HEIGHT 4
#define TEMPO_GRID_GRADIENT_HEIGHT 64
#define TEMPO_GRID_TEXT_WIDTH 64
#define TEMPO_GRID_TEXT_X_OFFSET 0
#define TEMPO_GRID_TEXT_Y_OFFSET -24
#define TEMPO_GRID_FONT_SIZE 12

TempoGrid::TempoGrid(ActivityManager &am) : activityManager(am)
{
    loopModeToggle = false;
    loopSectionStartFrame = 0;
    loopSectionStopFrame = 0;

    activityManager.registerTaskListener(this);
    viewPositionManager->attachViewPositionListener(this);
}

void TempoGrid::paint(juce::Graphics &g)
{
    auto bounds = g.getClipBounds();

    // lazy way to get a center line
    auto halfBounds = bounds.withY(bounds.getCentreY());
    auto centerLine = juce::Line<int>(halfBounds.getX(), halfBounds.getTopLeft().getY(),
                                      halfBounds.getX() + halfBounds.getWidth(), halfBounds.getTopLeft().getY());

    // we used to call paintMiddleGradient here but it doesn't really
    // fit in our new graphic chart

    paintLoopSection(g);

    // draw a horizontal line in the middle
    g.setColour(COLOR_TEXT_DARKER);
    g.drawLine(centerLine.toFloat(), 1);

    // now draw one tick at every bar
    paintTicks(g, bounds, halfBounds);
}

void TempoGrid::paintMiddleGradient(juce::Graphics &g, juce::Rectangle<int> halfBounds)
{
    // draw a gradient in the middle
    juce::ColourGradient topGradient(
        juce::Colours::transparentBlack, halfBounds.getX(), halfBounds.getTopLeft().getY() - TEMPO_GRID_GRADIENT_HEIGHT,
        COLOR_BACKGROUND.withAlpha(0.5f), halfBounds.getX(), halfBounds.getTopLeft().getY(), false);

    g.setGradientFill(topGradient);
    g.fillRect(halfBounds.getX(), halfBounds.getTopLeft().getY() - TEMPO_GRID_GRADIENT_HEIGHT, halfBounds.getWidth(),
               TEMPO_GRID_GRADIENT_HEIGHT);

    juce::ColourGradient botGradient(COLOR_BACKGROUND.withAlpha(0.5f), halfBounds.getX(),
                                     halfBounds.getTopLeft().getY(), juce::Colours::transparentBlack, halfBounds.getX(),
                                     halfBounds.getTopLeft().getY() + TEMPO_GRID_GRADIENT_HEIGHT, false);

    g.setGradientFill(botGradient);
    g.fillRect(halfBounds.getX(), halfBounds.getTopLeft().getY(), halfBounds.getWidth(), TEMPO_GRID_GRADIENT_HEIGHT);
}

void TempoGrid::paintTicks(juce::Graphics &g, juce::Rectangle<int> bounds, juce::Rectangle<int> halfBounds)
{
    // width of a tempo bar in audio frame
    float barFrameWidth = float(AUDIO_FRAMERATE * 60) / tempo;

    // width of a tempo bar in pixels
    float barPixelWidth = barFrameWidth / viewPositionManager->getViewScale();

    // what's the position of the view in bars
    float viewStartBarIndex = viewPositionManager->getViewPosition() / barFrameWidth;

    float barPixelShift = barPixelWidth * (1.0f - (viewStartBarIndex - std::floor(viewStartBarIndex)));

    int firstDisplayedBar = ((int)viewStartBarIndex) + 1;

    int noBars = float(bounds.getWidth()) / barPixelWidth;

    auto barTickLine = juce::Line<float>();
    auto barTickNumberArea = juce::Rectangle<float>();

    g.setFont(juce::Font(TEMPO_GRID_FONT_SIZE));

    // now we prepare to draw the 4x4 sub tempo bars
    float subBarPixelWidth = barPixelWidth / 4.0f;
    float subBarFrameWidth = barFrameWidth / 4.0f;
    float subBarViewIndex = viewPositionManager->getViewPosition() / subBarFrameWidth;
    float subBarPixelShift = subBarPixelWidth * (1.0f - (subBarViewIndex - std::floor(subBarViewIndex)));

    for (int i = 0; i <= noBars * 4; i++)
    {

        if ((viewStartBarIndex * 4) + i < 4)
        {
            continue;
        }

        float xOrigin = bounds.getX() + subBarPixelShift + float(i) * subBarPixelWidth;
        barTickLine.setStart(xOrigin, halfBounds.getY() - SUBBAR_LINE_HEIGHT);
        barTickLine.setEnd(xOrigin, halfBounds.getY() + SUBBAR_LINE_HEIGHT);
        g.drawLine(barTickLine, 2);
    }

    // draw wider bars
    float barWidth;
    float barHighlight;
    float eightBarsQueue;

    for (int i = 0; i <= noBars; i++)
    {
        eightBarsQueue = 0;

        if ((firstDisplayedBar + i) % 8 == 1)
        {
            g.setColour(COLOR_LABELS_BORDER);
            barWidth = 3;
            barHighlight = 3;
        }
        else
        {
            g.setColour(COLOR_TEXT_DARKER);
            barWidth = 2;
            barHighlight = 0;
        }

        if ((firstDisplayedBar + i) % (8 * 4) == 1)
        {
            g.setColour(juce::Colour(210, 170, 170));
            eightBarsQueue = 4;
        }

        float xOrigin = bounds.getX() + barPixelShift + float(i) * barPixelWidth;
        barTickLine.setStart(xOrigin, halfBounds.getY() - BAR_LINE_HEIGHT - barHighlight);
        barTickLine.setEnd(xOrigin, halfBounds.getY() + BAR_LINE_HEIGHT + barHighlight + eightBarsQueue);
        g.drawLine(barTickLine, barWidth);

        barTickNumberArea.setX(xOrigin - (TEMPO_GRID_TEXT_WIDTH / 2));
        barTickNumberArea.setWidth(TEMPO_GRID_TEXT_WIDTH);
        barTickNumberArea.setY(halfBounds.getY() + TEMPO_GRID_TEXT_Y_OFFSET - barHighlight);
        barTickNumberArea.setHeight(TEMPO_GRID_TEXT_WIDTH);

        g.setColour(COLOR_TEXT_DARKER);
        g.drawText(std::to_string(firstDisplayedBar + i), barTickNumberArea, juce::Justification::centredTop, false);
    }
}

void TempoGrid::updateTempo(float newTempo)
{
    tempo = newTempo;
}

void TempoGrid::paintLoopSection(juce::Graphics &g)
{
    int screenWidth = getLocalBounds().getWidth();

    int64_t viewFrameWidth = screenWidth * viewPositionManager->getViewScale();

    float loopStartScreenPosProportion =
        (float(loopSectionStartFrame) - float(viewPositionManager->getViewPosition())) / float(viewFrameWidth);
    float loopStopScreenPosProportion =
        (float(loopSectionStopFrame) - float(viewPositionManager->getViewPosition())) / float(viewFrameWidth);

    // if loop has a length of zero, we don't draw anything
    if (loopSectionStartFrame == loopSectionStopFrame)
    {
        return;
    }

    if (loopModeToggle)
    {
        paintColoredLoopOutline(g, loopStartScreenPosProportion, loopStopScreenPosProportion);
    }

    paintLoopBorder(g, loopStartScreenPosProportion, loopStopScreenPosProportion);
}

void TempoGrid::paintLoopBorder(juce::Graphics &g, float loopStartScreenPosProportion,
                                float loopStopScreenPosProportion)
{
    int screenWidth = getLocalBounds().getWidth();
    int screenHeight = getLocalBounds().getHeight();

    // if the loop section is visible on screen
    if (loopStartScreenPosProportion <= 1.0 && loopStopScreenPosProportion >= 0.0)
    {
        // draw the line that covers the loop section
        g.setColour(COLOR_LOOP_SECTION);
        g.drawLine(loopStartScreenPosProportion * screenWidth, screenHeight, loopStopScreenPosProportion * screenWidth,
                   screenHeight, LOOP_SECTION_LINE_WIDTH);

        // define area for the icons on the edges
        int iconWidth = 0.75f * LOOP_SECTION_BORDERS_RECT_WIDTH;
        juce::Rectangle<int> iconsRectangle(iconWidth, iconWidth);

        // draw the two rectangles at the edges of the loop section
        juce::Rectangle<int> bordersRect = getLoopHandleArea(true);
        g.fillRect(bordersRect);
        iconsRectangle.setCentre(bordersRect.getCentre());
        sharedIcons->moveIcon->drawWithin(g, iconsRectangle.toFloat(), juce::RectanglePlacement::centred, 1.0f);

        bordersRect = getLoopHandleArea(false);
        g.fillRect(bordersRect);
        iconsRectangle.setCentre(bordersRect.getCentre());
        sharedIcons->resizeHorizontalIcon->drawWithin(g, iconsRectangle.toFloat(), juce::RectanglePlacement::centred,
                                                      1.0f);
    }
}

void TempoGrid::paintColoredLoopOutline(juce::Graphics &g, float loopStartScreenPosProportion,
                                        float loopStopScreenPosProportion)
{
    int screenWidth = getLocalBounds().getWidth();
    int screenHeight = getLocalBounds().getHeight();

    juce::Rectangle<int> outlineArea(0, screenHeight);
    outlineArea.setY(0);
    outlineArea.setX(0);
    g.setColour(COLOR_LOOP_SECTION.withAlpha(0.2f));

    if (loopStartScreenPosProportion > 0.0f)
    {
        float outlineEnd = juce::jmin(1.0f, loopStartScreenPosProportion);
        outlineArea.setWidth(outlineEnd * screenWidth);
        g.fillRect(outlineArea);
    }

    if (loopStopScreenPosProportion < 1.0f)
    {
        float outlineStart = juce::jmax(0.0f, loopStopScreenPosProportion);
        outlineArea.setWidth((1.0f - outlineStart) * screenWidth);
        outlineArea.setX(outlineStart * screenWidth);
        g.fillRect(outlineArea);
    }
}

bool TempoGrid::hitTest(int x, int y)
{
    if (activityManager.getAppState().getUiState() == UI_STATE_MOVE_LOOP_SECTION)
    {
        return true;
    }

    auto leftHandle = getLoopHandleArea(true);
    auto rightHandle = getLoopHandleArea(false);

    if (leftHandle.contains(x, y) || rightHandle.contains(x, y))
    {
        return true;
    }

    return false;
}

juce::Rectangle<int> TempoGrid::getLoopHandleArea(bool left)
{
    int screenWidth = getLocalBounds().getWidth();
    int screenHeight = getLocalBounds().getHeight();
    int64_t viewFrameWidth = screenWidth * viewPositionManager->getViewScale();

    juce::Rectangle<int> bordersRect(LOOP_SECTION_BORDERS_RECT_WIDTH, LOOP_SECTION_BORDERS_RECT_HEIGHT);
    bordersRect.setY(screenHeight - LOOP_SECTION_BORDERS_RECT_HEIGHT);

    float handleScreenPos;
    if (left)
    {
        handleScreenPos =
            (float(loopSectionStartFrame) - float(viewPositionManager->getViewPosition())) / float(viewFrameWidth);
    }
    else
    {
        handleScreenPos =
            (float(loopSectionStopFrame) - float(viewPositionManager->getViewPosition())) / float(viewFrameWidth);
    }

    bordersRect.setX((handleScreenPos * screenWidth) - (LOOP_SECTION_BORDERS_RECT_WIDTH / 2));
    return bordersRect;
}

bool TempoGrid::taskHandler(std::shared_ptr<Task> task)
{
    // receives updates about completed tasks for loop state change
    auto loopToggleTask = std::dynamic_pointer_cast<LoopToggleTask>(task);
    if (loopToggleTask != nullptr && loopToggleTask->isCompleted())
    {
        loopModeToggle = loopToggleTask->isCurrentlyLooping;
        repaint();
        return false;
    }

    // receives updates about completed tasks for loop position change
    auto loopPositionTask = std::dynamic_pointer_cast<LoopMovingTask>(task);
    if (loopPositionTask != nullptr && loopPositionTask->isCompleted())
    {
        loopSectionStartFrame = loopPositionTask->currentLoopBeginFrame;
        loopSectionStopFrame = loopPositionTask->currentLoopEndFrame;
        repaint();
        return false;
    }

    return false;
}

void TempoGrid::mouseDown(const juce::MouseEvent &me)
{
    if (activityManager.getAppState().getUiState() == UI_STATE_DEFAULT && me.mods.isLeftButtonDown())
    {
        auto leftHandle = getLoopHandleArea(true);
        auto rightHandle = getLoopHandleArea(false);

        if (rightHandle.contains(me.getPosition().getX(), me.getPosition().getY()))
        {
            activityManager.getAppState().setUiState(UI_STATE_MOVE_LOOP_SECTION);
            draggingLeftHandle = false;
            setMouseCursor(juce::MouseCursor::RightEdgeResizeCursor);
            currentDragTask = std::make_shared<LoopMovingTask>(loopSectionStartFrame, loopSectionStopFrame,
                                                               loopSectionStartFrame, loopSectionStopFrame);
            return;
        }

        if (leftHandle.contains(me.getPosition().getX(), me.getPosition().getY()))
        {
            activityManager.getAppState().setUiState(UI_STATE_MOVE_LOOP_SECTION);
            draggingLeftHandle = true;
            setMouseCursor(juce::MouseCursor::DraggingHandCursor);
            currentDragTask = std::make_shared<LoopMovingTask>(loopSectionStartFrame, loopSectionStopFrame,
                                                               loopSectionStartFrame, loopSectionStopFrame);
            return;
        }
    }
}

void TempoGrid::mouseUp(const juce::MouseEvent &me)
{
    if (me.mods.isLeftButtonDown() && activityManager.getAppState().getUiState() == UI_STATE_MOVE_LOOP_SECTION)
    {
        activityManager.getAppState().setUiState(UI_STATE_DEFAULT);
        setMouseCursor(juce::MouseCursor::NormalCursor);
        currentDragTask->currentLoopBeginFrame = loopSectionStartFrame;
        currentDragTask->currentLoopEndFrame = loopSectionStopFrame;
        currentDragTask->setCompleted(true);
        activityManager.broadcastTask(currentDragTask);
    }
}

void TempoGrid::mouseDrag(const juce::MouseEvent &me)
{
    if (activityManager.getAppState().getUiState() == UI_STATE_MOVE_LOOP_SECTION)
    {
        int64_t mouseFramePosition =
            viewPositionManager->getViewPosition() + (me.getPosition().getX() * viewPositionManager->getViewScale());

        std::shared_ptr<LoopMovingTask> task;

        // left handle is the one that moves, right one resizes
        if (draggingLeftHandle)
        {
            int64_t loopFrameSize = loopSectionStopFrame - loopSectionStartFrame;
            task = std::make_shared<LoopMovingTask>(mouseFramePosition, mouseFramePosition + loopFrameSize);
        }
        else
        {
            int64_t loopFrameSize = mouseFramePosition - loopSectionStartFrame;
            task = std::make_shared<LoopMovingTask>(loopSectionStartFrame, loopSectionStartFrame + loopFrameSize);
        }

        task->quantisize(float(60 * AUDIO_FRAMERATE) / float(tempo));

        if (task->currentLoopBeginFrame != loopSectionStartFrame || task->currentLoopEndFrame != loopSectionStopFrame)
        {
            activityManager.broadcastTask(task);
        }
    }
}

void TempoGrid::mouseMove(const juce::MouseEvent &me)
{
    if (activityManager.getAppState().getUiState() == UI_STATE_DEFAULT)
    {
        auto leftHandle = getLoopHandleArea(true);
        auto rightHandle = getLoopHandleArea(false);

        if (rightHandle.contains(me.getPosition().getX(), me.getPosition().getY()))
        {
            setMouseCursor(juce::MouseCursor::RightEdgeResizeCursor);
            return;
        }

        if (leftHandle.contains(me.getPosition().getX(), me.getPosition().getY()))
        {
            setMouseCursor(juce::MouseCursor::DraggingHandCursor);
            return;
        }

        setMouseCursor(juce::MouseCursor::NormalCursor);
    }
}

void TempoGrid::viewPositionUpdateCallback()
{
    repaint();
}