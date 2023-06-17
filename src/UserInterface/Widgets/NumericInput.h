#ifndef DEF_NUMERIC_INPUT_HPP
#define DEF_NUMERIC_INPUT_HPP

#include "../../Arrangement/ActivityManager.h"
#include "../CustomFonts.h"

#include <juce_gui_basics/juce_gui_basics.h>

#define NUMERIC_INPUT_TEXT_MARGINS 3
#define NUMERIC_INPUT_SIDE_ADDITIONAL_MARGINS 8

#define FORMAT_BUFFER_MAXLEN 128

/**
 * @brief      This class describes a numeric input component. It is assigned
 *             a potentially changeable NumericInputId (@see NumericInputIdManager)
 *             and will emit and receive events so that it can update the
 *             input values againt the service it is recorded at.
 */
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

    // the shared fonts
    juce::SharedResourcePointer<CustomFonts> sharedFonts;

    // the identifier of the numeric input to pop appropriate update tasks
    int numericInputId;

    // the activity manager we need to broadcast tasks
    ActivityManager *activityManager;

    // the buffer holding formatted string
    char formatBuffer[FORMAT_BUFFER_MAXLEN];

    /**
     * @brief      Return the stringified value of this numeric input.
     *
     * @return     The string value.
     */
    std::string getStringValue();

    /**
     * @brief      Fetches the numeric input value if possible.
     *             It's possible if both activityManager and numericInputId
     *             were set. Note that if won't happen if the input get started
     *             before the object holding the value and responding to the event
     *             hasn't been registered as task listener yet. This is likely to happen
     *             but the said object will also broadcast its value on startup so we'
     *             this procedure ensure we're safe both ways.
     */
    void fetchValueIfPossible();
};

#endif // DEF_NUMERIC_INPUT_HPP