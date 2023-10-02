#include "TimeQuantization.h"
#include "ActivityManager.h"
#include "NumericInputId.h"
#include "Task.h"
#include <memory>

TimeQuantization::TimeQuantization(ActivityManager &am) : tempo(DEFAULT_TEMPO), activityManager(am)
{
}

bool TimeQuantization::taskHandler(std::shared_ptr<Task> task)
{
    auto tempoTask = std::dynamic_pointer_cast<NumericInputUpdateTask>(task);
    if (tempoTask != nullptr && tempoTask->numericalInputId == NUM_INPUT_ID_TEMPO && !tempoTask->isCompleted() &&
        !tempoTask->hasFailed())
    {

        // if it's just a broadcasting request, simply broadcast existing value
        if (tempoTask->isBroadcastRequest)
        {
            tempoTask->newValue = tempo;
        }

        // if this task wants to set a new value, well set it if possible
        else if (tempoTask->newValue <= MAX_TEMPO && tempoTask->newValue >= MIN_TEMPO)
        {
            // +0.5f to round instead of truncating
            tempo = int(tempoTask->newValue + 0.5f);
        }

        tempoTask->setCompleted(true);
        activityManager.broadcastNestedTaskNow(tempoTask);
        return true;
    }

    return false;
}

int TimeQuantization::getTempo() const
{
    return tempo;
}

void TimeQuantization::reset()
{
    tempo = DEFAULT_TEMPO;

    auto tempoUpdateTask = std::make_shared<NumericInputUpdateTask>(NUM_INPUT_ID_TEMPO, float(tempo));
    activityManager.broadcastNestedTaskNow(tempoUpdateTask);
}