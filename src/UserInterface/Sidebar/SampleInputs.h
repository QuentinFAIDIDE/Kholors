#ifndef DEF_SAMPLE_INPUTS_HPP
#define DEF_SAMPLE_INPUTS_HPP

#include "../Widgets/NumericInput.h"

/**
 * @brief      Describes a numerical input component tied
 *             to a sample fade in or out parameters in ms.
 */
class SampleFadeInput : public NumericInput
{
  public:
    /**
     * @brief      Constructs a new instance.
     *
     * @param[in]  isFadeInNotOut  Indicates if fade in
     */
    SampleFadeInput(bool isFadeInNotOut);

    /**
     * @brief      Sets the sample id to modify fade in or out for.
     *             Set it to -1 to disable task broadcasting.
     *
     * @param[in]  ids   set of identifier of the samples to change
     *
     */
    void setSampleIds(std::set<size_t> &ids);

    /**
     * @brief      if possible (activity manager is set and id != -1) broadcast
     *             a task to get broadcasted a completed task with the value
     *             of target fade in or out in ms.
     */
    void fetchValueIfPossible() override;

    /**
     * @brief      Parses tasks to receive value updates.
     *
     * @param[in]  task  The task
     *
     * @return     true to stop task broadcasting (for this class, we return false
     *             so that concurrent input component can function)
     */
    bool taskHandler(std::shared_ptr<Task> task) override;

    /**
     * @brief      Emits the final drag task. See parent class doc. @see NumericInput
     */
    void emitFinalDragTask() override;

    /**
     * @brief      Emits the intermediate drag task. See parent class doc. @see NumericInput
     */
    void emitIntermediateDragTask(float value) override;

  private:
    void startDragging() override;

    // the identifier of the sample we are updating things for
    std::set<size_t> sampleIds;

    // map of values when the dragging movements begins (for all selected samples)
    std::map<size_t, int> initialValues;

    // values that are currently set by the input (not all samples have same values
    // since some may fail to update past a certains thresold (ex: fade in larger than some sample))
    std::map<size_t, int> currentValues;

    // the sample id in the selected ones that drives the displayed values
    int displayedSampleId;

    // a boolean to know if we are currently iterating over selection
    bool iteratingOverSelection;

    // are we fading in or out ?
    bool isFadeIn;
};

/////////////////////////////////////////////////////////

class SampleGainInput : public NumericInput
{
  public:
    /**
     * @brief      Constructs a new instance.
     *
     */
    SampleGainInput();

    /**
     * @brief      Sets the sample id to modify fade in or out for.
     *             Set it to -1 to disable task broadcasting.
     *
     * @param[in]  ids   set of identifier of the samples to change
     *
     */
    void setSampleIds(std::set<size_t> &ids);

    /**
     * @brief      if possible (activity manager is set and id != -1) broadcast
     *             a task to get broadcasted a completed task with the value
     *             of target db value.
     */
    void fetchValueIfPossible() override;

    /**
     * @brief      Parses tasks to receive value updates.
     *
     * @param[in]  task  The task
     *
     * @return     true to stop task broadcasting (for this class, we return false
     *             so that concurrent input component can function)
     */
    bool taskHandler(std::shared_ptr<Task> task) override;

    /**
     * @brief      Emits the final drag task. See parent class doc. @see NumericInput
     */
    void emitFinalDragTask() override;

    /**
     * @brief      Emits the intermediate drag task. See parent class doc. @see NumericInput
     */
    void emitIntermediateDragTask(float value) override;

  private:
    // called when the input is dragged
    void startDragging() override;

    // the identifier of the sample we are updating things for
    std::set<size_t> sampleIds;

    // map of values when the dragging movements begins (for all selected samples)
    std::map<size_t, int> initialValues;

    // values that are currently set by the input (not all samples have same values
    // since some may fail to update past a certains thresold (ex: fade in larger than some sample))
    std::map<size_t, int> currentValues;

    // the sample id in the selected ones that drives the displayed values
    int displayedSampleId;

    // a boolean to know if we are currently iterating over selection
    bool iteratingOverSelection;
};

/////////////////////////////////////////////////////////

class SampleFilterRepeatInput : public NumericInput
{
  public:
    /**
     * @brief      Constructs a new instance.
     *
     * @param[in]  isLp  boolean true if it's for a lowpass filter, false for highpass.
     */
    SampleFilterRepeatInput(bool isLp);

    /**
     * @brief      Sets the sample ids to modify values on.
     *             Set it to -1 to disable task broadcasting.
     *
     * @param[in]  ids   set of identifier of the samples to change
     *
     */
    void setSampleIds(std::set<size_t> &ids);

    /**
     * @brief      if possible (activity manager is set and id != -1) broadcast
     *             a task to get broadcasted a completed task with the value
     *             of target db value.
     */
    void fetchValueIfPossible() override;

    /**
     * @brief      Parses tasks to receive value updates.
     *
     * @param[in]  task  The task
     *
     * @return     true to stop task broadcasting (for this class, we return false
     *             so that concurrent input component can function)
     */
    bool taskHandler(std::shared_ptr<Task> task) override;

    /**
     * @brief      Emits the final drag task. See parent class doc. @see NumericInput
     */
    void emitFinalDragTask() override;

    /**
     * @brief      Emits the intermediate drag task. See parent class doc. @see NumericInput
     */
    void emitIntermediateDragTask(float value) override;

  private:
    // called when the input is dragged
    void startDragging() override;

    // the identifier of the sample we are updating things for
    std::set<size_t> sampleIds;

    // map of values when the dragging movements begins (for all selected samples)
    std::map<size_t, int> initialValues;

    // values that are currently set by the input (not all samples have same values
    // since some may fail to update past a certains thresold (ex: fade in larger than some sample))
    std::map<size_t, int> currentValues;

    // the sample id in the selected ones that drives the displayed values
    int displayedSampleId;

    // a boolean to know if we are currently iterating over selection
    bool iteratingOverSelection;

    // true if this modify low pass repeat, false if for high pass
    bool isForLowPassFilter;
};

#endif // DEF_SAMPLE_INPUTS_HPP