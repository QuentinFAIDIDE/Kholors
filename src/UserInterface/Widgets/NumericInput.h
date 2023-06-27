#ifndef DEF_NUMERIC_INPUT_HPP
#define DEF_NUMERIC_INPUT_HPP

#include "../../Arrangement/ActivityManager.h"
#include "../FontsLoader.h"

#include <juce_gui_basics/juce_gui_basics.h>

#define NUMERIC_INPUT_TEXT_MARGINS 3
#define NUMERIC_INPUT_SIDE_ADDITIONAL_MARGINS 2
#define NUMERIC_INPUT_MIN_DRAG_UPDATE 4.0f
#define FORMAT_BUFFER_MAXLEN 128

class NumericInput : public juce::Component, public TaskListener
{
  public:
    /**
     * @brief      Constructs a new instance.
     *
     * @param[in]  integers   Should we treat values as integers ?
     * @param[in]  minValue   The minimum value
     * @param[in]  maxValue   The maximum value
     * @param[in]  stepValue  The step value
     */
    NumericInput(bool integers, float minValue, float maxValue, float stepValue);

    /**
     * @brief      paints the widget content
     */
    void paint(juce::Graphics &g) override;

    /**
     * @brief      Sets the activity manager. Note that initial value update won't happen if this is called
     *             before the object holding the value and responding to the event
     *             hasn't been registered as task listener yet. This is likely to happen
     *             but the said object will also broadcast its value on startup so we follow
     *             this procedure to ensure we're safe whoever starts first.
     *
     * @param      am    The new value
     */
    void setActivityManager(ActivityManager *am);

    /**
     * @brief      Set the unit str to be displayed after the numeric value.
     *             Using "" will disable the unit displaying.
     *
     * @param[in]  newUnit  The new unit text
     */
    void setUnit(std::string newUnit);

    /**
     * @brief      Parses tasks to match the relevant ones (in general, completed ones of
     *             the relevant task class) and update the input value. It's a good idea
     *             to return false in order to allow multiple inputs to handle the same task (the
     *             return value decides if the tasks broacasting stops or continue to other
     *             TaskListerners). It should call setValue.
     *
     * @param[in]  task  The task
     *
     * @return     true if we handled a task and it doesn't need further
     *             treatement by other TaskListener.
     */
    virtual bool taskHandler(std::shared_ptr<Task> task) override = 0;

    /**
     * @brief      Called when mouse is clicked.
     *
     */
    void mouseDown(const juce::MouseEvent &) override;

    /**
     * @brief      Called when mouse click is released.
     *
     */
    void mouseUp(const juce::MouseEvent &) override;

    /**
     * @brief      Called when mouse is moved while clicking.
     *
     */
    void mouseDrag(const juce::MouseEvent &) override;

    /**
     * @brief      Sets the value of the input.
     *
     * @param[in]  value  The value
     */
    void setValue(float value);

    /**
     * @brief      Return the current input value. Note
     *             that the value is updated in the taskHandler
     *             upon receiving a completed task that was
     *             first emmited by emitIntermediateDragTask.
     *
     * @return     The value.
     */
    float getValue();

    /**
     * @brief      Broadcast a task that will try to update the value
     *             in the class with the corresponding taskHandler.
     *             On success, a completed task will be received
     *             in the taskHandler. This is for intermediate update
     *             while the user is dragging the input value, it should
     *             not record the task in history.
     *
     * @param[in]  value  The value
     */
    virtual void emitIntermediateDragTask(float value) = 0;

    /**
     * @brief      Emits an already completed task that is to be recorded
     *             in task history and that can be reverted. It should use
     *             the initial drag value that you can get with getInitialDragValue()
     *             and the current one from getValue().
     *             This is called when user relieves the mouse button and therefore
     *             completes a dragging movement.
     */
    virtual void emitFinalDragTask() = 0;

    /**
     * @brief      Return the value of the input at the beginning
     *             of the user drag movement.
     *
     * @return     The initial drag value.
     */
    float getInitialDragValue();

    /**
     * @brief      Should emit a task that will let the relevant TaskListener
     *             broadcast a completed on that can be parsed by our taskHandler.
     *             You should take care that activityManager is set and eventually
     *             additional setting of yours are ok.
     */
    virtual void fetchValueIfPossible() = 0;

    /**
     * @brief      Return current activity manager or nullptr.
     *
     * @return     The activity manager.
     */
    ActivityManager *getActivityManager();

  private:
    // the actual displayed value
    float value;
    // are we using integers or float ?
    bool isInteger;
    // minimum value the input can take
    float min;
    // maximum value the input can take
    float max;
    // what's the lower unit of modification
    float step;

    // width of a monospace character
    float charWidth;

    // the initial value when a drag was initiated
    float dragInitialValue;

    // the shared fonts
    juce::SharedResourcePointer<FontsLoader> sharedFonts;

    // the activity manager we need to broadcast tasks
    ActivityManager *activityManager;

    // are we currently dragging this component value ?
    bool isDragging;

    // the last position at which the mouse drag initatied a value update
    int lastDragUpdateY;

    // the buffered mouse position stored when the udpate task is emmited
    // and used elswhere when the task is received
    int pendingDragUpdateY;

    // the buffer holding formatted string
    char formatBuffer[FORMAT_BUFFER_MAXLEN];

    // text of the unit to display, defaults to empty
    std::string unit;

    //////////////////////////////////////////////////

    /**
     * @brief      Return the stringified value of this numeric input.
     *
     * @return     The string value.
     */
    std::string getStringValue();
};

/**
 * @brief      This class describes a generic numeric input component. It is assigned
 *             a potentially changeable NumericInputId (@see NumericInputIdManager)
 *             and will emit and receive NumericInputUpdateTask tasks so that it can update the
 *             input values againt the service they are recorded at.
 *             This was made to handle generic inputs.
 */
class GenericNumericInput : public NumericInput
{
  public:
    /**
     * @brief      Constructs a new instance. Should will call parent initalizer.
     *
     * @param[in]  integers   Should we treat values as integers ?
     * @param[in]  minValue   The minimum value
     * @param[in]  maxValue   The maximum value
     * @param[in]  stepValue  The step value
     */
    GenericNumericInput(bool integers, float minValue, float maxValue, float stepValue);

    /**
     * @brief      Sets the numeric input identifier. Note that initial value update won't happen if this is called
     *             before the object holding the value and responding to the event
     *             hasn't been registered as task listener yet. This is likely to happen
     *             but the said object will also broadcast its value on startup so we follow
     *             this procedure to ensure we're safe whoever starts first.
     */
    void setNumericInputId(int id);

    /**
     * @brief      Recevies task and await for one with matching
     *             numerical input id that are completed to update
     *             its value.
     *
     * @param[in]  task  The task
     *
     * @return     true if we handled a task and it doesn't need further
     *             treatement by other TaskListener.
     */
    bool taskHandler(std::shared_ptr<Task> task) override;

    /**
     * @brief      Broadcast a task that will try to update the value
     *             in the class with the corresponding taskHandler.
     *             On success, a completed task will be received
     *             in the taskHandler. This is for intermediate update
     *             while the user is dragging the input value, it should
     *             not record the task in history.
     *
     * @param[in]  value  The value
     */
    void emitIntermediateDragTask(float value) override;

    /**
     * @brief      Emits an already completed task that is to be recorded
     *             in task history and that can be reverted. It should use
     *             the initial drag value that you can get with getInitialDragValue()
     *             and the current one from getValue.
     *             This is called when user relieves the mouse button and therefore
     *             completes a dragging movement.
     */
    void emitFinalDragTask() override;

    /**
     * @brief      Fetches the numeric input value if possible.
     *             It's possible if both activityManager and numericInputId
     *             were set. Note that if won't happen if the input get started
     *             before the object holding the value and responding to the event
     *             hasn't been registered as task listener yet. This is likely to happen
     *             but the said object will also broadcast its value on startup so we'
     *             this procedure ensure we're safe both ways.
     */
    void fetchValueIfPossible() override;

  private:
    // the identifier of the numeric input to pop appropriate update tasks
    int numericInputId;
};

#endif // DEF_NUMERIC_INPUT_HPP