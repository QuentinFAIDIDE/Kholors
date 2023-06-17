#include "NumericInput.h"
#include "../../Config.h"
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>

NumericInput::NumericInput(bool integers, float minValue, float maxValue, float stepValue)
    : value(0.0f), isInteger(integers), min(minValue), max(maxValue), step(stepValue), numericInputId(-1),
      activityManager(nullptr), isDragging(false), unit("")
{
    charWidth = sharedFonts->monospaceFont.withHeight(SMALLER_FONT_SIZE).getStringWidth(" ");
}

void NumericInput::paint(juce::Graphics &g)
{
    g.setColour(COLOR_BACKGROUND.withAlpha(0.5f));
    g.fillRoundedRectangle(g.getClipBounds().toFloat(), 3);

    juce::Line<int> bottomLine(g.getClipBounds().getBottomLeft(), g.getClipBounds().getBottomRight());
    g.setColour(COLOR_TEXT_DARKER.withAlpha(0.15f));
    g.drawLine(bottomLine.toFloat(), 2.0f);

    auto textArea = g.getClipBounds().reduced(NUMERIC_INPUT_TEXT_MARGINS + NUMERIC_INPUT_SIDE_ADDITIONAL_MARGINS,
                                              NUMERIC_INPUT_TEXT_MARGINS);

    g.setFont(sharedFonts->monospaceFont.withHeight(SMALLER_FONT_SIZE));

    if (unit != "")
    {
        g.setColour(COLOR_UNITS);
        auto unitArea = textArea.removeFromRight((charWidth * unit.length()));
        g.drawText(unit, unitArea, juce::Justification::centredRight);
    }

    g.setColour(COLOR_TEXT_DARKER);
    g.drawText(getStringValue(), textArea, juce::Justification::centredRight);
}

std::string NumericInput::getStringValue()
{
    // TODO: make format a parameter for how many digits after commas to display
    if (isInteger)
    {
        return std::to_string(int(value + 0.5f));
    }
    else
    {
        snprintf(formatBuffer, FORMAT_BUFFER_MAXLEN, "%.2f\n", value);
        return std::string(formatBuffer);
    }
}

void NumericInput::setActivityManager(ActivityManager *am)
{
    activityManager = am;
    fetchValueIfPossible();
}

void NumericInput::setNumericInputId(int id)
{
    numericInputId = id;
    fetchValueIfPossible();
}

void NumericInput::fetchValueIfPossible()
{
    if (activityManager != nullptr && numericInputId >= 0)
    {
        // emit a task that gets the initial value
        std::shared_ptr<NumericInputUpdateTask> task = std::make_shared<NumericInputUpdateTask>(numericInputId);
        activityManager->broadcastTask(task);
    }
}

void NumericInput::setUnit(std::string newUnit)
{
    unit = newUnit;
}

bool NumericInput::taskHandler(std::shared_ptr<Task> task)
{
    // no need to parse tasks if we have no assigned id
    if (numericInputId < 0)
    {
        return false;
    }

    // we are interested in completed NumericInputUpdateTask
    std::shared_ptr<NumericInputUpdateTask> updateTask = std::dynamic_pointer_cast<NumericInputUpdateTask>(task);
    if (updateTask != nullptr && updateTask->isCompleted() && !updateTask->hasFailed() &&
        updateTask->numericalInputId == numericInputId)
    {
        if (isInteger)
        {
            lastDragUpdateY = pendingDragUpdateY;
            value = std::round(updateTask->newValue);
        }
        else
        {
            lastDragUpdateY = pendingDragUpdateY;
            value = updateTask->newValue;
        }
        return true;
    }

    return false;
}

void NumericInput::mouseDown(const juce::MouseEvent &me)
{
    if (activityManager == nullptr)
    {
        return;
    }

    if (!isDragging && me.mods.isLeftButtonDown() && activityManager->getAppState().getUiState() == UI_STATE_DEFAULT)
    {
        lastDragUpdateY = me.getMouseDownY();
        isDragging = true;
        dragInitialValue = value;
        activityManager->getAppState().setUiState(UI_STATE_MOUSE_DRAG_NUMERIC_INPUT);
    }
}

void NumericInput::mouseUp(const juce::MouseEvent &me)
{
    if (activityManager == nullptr)
    {
        return;
    }

    if (isDragging && me.mods.isLeftButtonDown() &&
        activityManager->getAppState().getUiState() == UI_STATE_MOUSE_DRAG_NUMERIC_INPUT)
    {
        isDragging = false;
        activityManager->getAppState().setUiState(UI_STATE_DEFAULT);
        // emit the final task (already completed) so that we record it for reversion
        std::shared_ptr<NumericInputUpdateTask> task =
            std::make_shared<NumericInputUpdateTask>(numericInputId, value, dragInitialValue);
        activityManager->broadcastTask(task);
    }
}

void NumericInput::mouseDrag(const juce::MouseEvent &me)
{
    if (activityManager == nullptr)
    {
        return;
    }

    if (isDragging)
    {
        // if we had enough movement to trigger an update
        if (std::abs(me.getPosition().getY() - lastDragUpdateY) > NUMERIC_INPUT_MIN_DRAG_UPDATE)
        {
            // the new desired value
            float newValue =
                value - (step * (float(me.getPosition().getY() - lastDragUpdateY) / NUMERIC_INPUT_MIN_DRAG_UPDATE));

            pendingDragUpdateY = me.getPosition().getY();

            // try to pop a task that updates the value
            std::shared_ptr<NumericInputUpdateTask> task =
                std::make_shared<NumericInputUpdateTask>(numericInputId, newValue);
            activityManager->broadcastTask(task);
        }
    }
}