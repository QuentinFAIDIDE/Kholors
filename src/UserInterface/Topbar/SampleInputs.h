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
     * @param[in]  id   identifier of the sample to change
     *
     */
    void setSampleId(int id);

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
    // the identifier of the sample we are updating things for
    int sampleId;

    // are we fading in or out ?
    bool isFadeIn;
};

#endif // DEF_SAMPLE_INPUTS_HPP