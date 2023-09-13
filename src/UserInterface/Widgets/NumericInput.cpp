#include "NumericInput.h"
#include "../../Config.h"
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>

#include "../Dialogs/NumericInputSetDialog.h"

NumericInput::NumericInput(bool integers, float minValue, float maxValue, float stepValue)
    : value(0.0f), isInteger(integers), min(minValue), max(maxValue), step(stepValue),
      minDragUpdate(NUMERIC_INPUT_MIN_DRAG_UPDATE), activityManager(nullptr), isDragging(false), unit("")
{
    charWidth = sharedFonts->monospaceFont.withHeight(DEFAULT_FONT_SIZE).getStringWidth(" ");
}

void NumericInput::paint(juce::Graphics &g)
{
    auto textArea = g.getClipBounds().reduced(NUMERIC_INPUT_TEXT_MARGINS + NUMERIC_INPUT_SIDE_ADDITIONAL_MARGINS, 0);

    g.setFont(sharedFonts->monospaceFont.withHeight(DEFAULT_FONT_SIZE));

    if (unit != "")
    {
        if (isMouseOverOrDragging(true))
        {
            g.setColour(COLOR_HIGHLIGHT);
        }
        else
        {
            g.setColour(COLOR_UNITS);
        }

        auto unitArea = textArea.removeFromRight((charWidth * unit.length()));
        g.drawText(unit, unitArea, juce::Justification::centredRight);
    }

    if (isMouseOverOrDragging(true))
    {
        g.setColour(COLOR_HIGHLIGHT);
    }
    else
    {
        g.setColour(COLOR_TEXT_DARKER);
    }
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
        snprintf(formatBuffer, FORMAT_BUFFER_MAXLEN, "%.2f", value);
        return std::string(formatBuffer);
    }
}

void NumericInput::setActivityManager(ActivityManager *am)
{
    activityManager = am;
    fetchValueIfPossible();
}

void NumericInput::setUnit(std::string newUnit)
{
    unit = newUnit;
}

void NumericInput::setValue(float val)
{
    // this value is therefore not "pending" anymore and is recorded
    lastDragUpdateY = pendingDragUpdateY;

    // we update the displayed value
    if (isInteger)
    {
        value = std::round(val);
    }
    else
    {
        value = val;
    }
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
        startDragging();
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
        emitFinalDragTask();
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
            float newValue = value - (step * (float(me.getPosition().getY() - lastDragUpdateY) / minDragUpdate));

            pendingDragUpdateY = me.getPosition().getY();

            newValue = juce::jlimit(min, max, newValue);

            if (newValue != getValue())
            {
                // try to pop a task that updates the value
                emitIntermediateDragTask(newValue);
            }
        }
    }
}

void NumericInput::mouseDoubleClick(const juce::MouseEvent &)
{
    juce::DialogWindow::LaunchOptions launchOptions;
    launchOptions.dialogTitle = "Set the numeric input value";
    launchOptions.content.set(new NumericInputSetDialog(*activityManager, this), true);
    launchOptions.escapeKeyTriggersCloseButton = true;
    launchOptions.useNativeTitleBar = true;
    launchOptions.resizable = false;
    launchOptions.useBottomRightCornerResizer = false;
    launchOptions.launchAsync();
}

void NumericInput::setMinDragUpdate(float v)
{
    minDragUpdate = v;
}

float NumericInput::getInitialDragValue()
{
    return dragInitialValue;
}

float NumericInput::getValue()
{
    return value;
}

ActivityManager *NumericInput::getActivityManager()
{
    return activityManager;
}

void NumericInput::startDragging()
{
}

/////////////////////////////////////

GenericNumericInput::GenericNumericInput(bool integers, float minValue, float maxValue, float stepValue)
    : NumericInput(integers, minValue, maxValue, stepValue), numericInputId(-1)
{
}

void GenericNumericInput::setNumericInputId(int id)
{
    numericInputId = id;
    fetchValueIfPossible();
}

void GenericNumericInput::fetchValueIfPossible()
{
    if (getActivityManager() != nullptr && numericInputId >= 0)
    {
        // emit a task that gets the initial value
        std::shared_ptr<NumericInputUpdateTask> task = std::make_shared<NumericInputUpdateTask>(numericInputId);
        getActivityManager()->broadcastTask(task);
    }
}

bool GenericNumericInput::taskHandler(std::shared_ptr<Task> task)
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
        setValue(updateTask->newValue);
        // we won't prevent event from being broadcasted further to allow for multiple inputs
        // to work on the same numeric id
        return false;
    }

    return false;
}

void GenericNumericInput::emitFinalDragTask()
{
    // emit the final task (already completed) so that we record it for reversion
    std::shared_ptr<NumericInputUpdateTask> task =
        std::make_shared<NumericInputUpdateTask>(numericInputId, getValue(), getInitialDragValue());
    getActivityManager()->broadcastTask(task);
}

void GenericNumericInput::emitIntermediateDragTask(float newValue)
{
    std::shared_ptr<NumericInputUpdateTask> task = std::make_shared<NumericInputUpdateTask>(numericInputId, newValue);
    getActivityManager()->broadcastTask(task);
}

bool GenericNumericInput::isValueValid(float)
{
    // basically, this component is not responsible
    // for the value and doesn't know if the provided one
    // is valid. It will just fail if it is and not get updated.
    // For simplicity's sake, I will not implement a procedure that
    // would communicate through a task to test if the value can be
    // entered, and will just let user click "yes" and see no updates.
    // If I feel like it's really worth it I might change my mind later.
    // NOTE: in the end, due to existing NumericInput design for str validation
    // on base class which currently use regexp to grey out button (hello fragile base class),
    // I am delaying implementation for that purpose and use this func
    // to ignore tasks instead of greying out confirm button.
    return true;
}

void GenericNumericInput::emitTaskToSetValue(float v)
{
    std::shared_ptr<NumericInputUpdateTask> task =
        std::make_shared<NumericInputUpdateTask>(numericInputId, v, getValue());
    task->setCompleted(false);
    getActivityManager()->broadcastTask(task);
}