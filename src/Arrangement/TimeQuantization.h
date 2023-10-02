#ifndef DEF_TIME_QUANTIZATION_HPP
#define DEF_TIME_QUANTIZATION_HPP

#include "TaskListener.h"
#include <memory>

class ActivityManager;

// This class holds tempo value, and is in charge
// of returning number of frames per sections and other
// quantization related values. Could be used in the future
// to provide a grid on which to quantizise values.
class TimeQuantization : public TaskListener
{
  public:
    /**
     * @brief Construct a new Time Quantization object
     */
    TimeQuantization(ActivityManager &am);

    /**
     * @brief Callback for the tasks broadcasted around the app. It
     *        will be used here to catch the tempo update task
     *        and when treated, broadcast their completed counterpart.
     *
     * @return true When the task should directly go to history after callback complete.
     * @return false When the task can continue down the list of TaskListeners.
     */
    bool taskHandler(std::shared_ptr<Task>) override;

    /**
     * @brief Return the tempo in beats per minutes.
     *
     * @return int Tempo in beats (bars) per minute.
     */
    int getTempo() const;

    /**
     * @brief Set the tempo to its default value and broadcast it.
     *        It expects to be called from another task and should not
     *        be called outside of task handler code (because it calls broadcastNestedTaskNow).
     */
    void reset();

  private:
    int tempo;                        /**< Global tempo in beats per minutes. */
    ActivityManager &activityManager; /**< Reference to the object that broadcasts tasks. */
};

#endif // DEF_TIME_QUANTIZATION_HPP