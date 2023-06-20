#include "LoopButton.h"
#include <memory>

LoopButton::LoopButton(ActivityManager &am) : activityManager(am)
{
    isLooping = false;
    activityManager.registerTaskListener(this);
}

bool LoopButton::taskHandler(std::shared_ptr<Task> task)
{
    auto loopUpdateTask = std::dynamic_pointer_cast<LoopToggleTask>(task);
    if (loopUpdateTask != nullptr && loopUpdateTask->isCompleted())
    {
        isLooping = loopUpdateTask->isCurrentlyPlaying;
        return false;
    }

    return false;
}

void LoopButton::paint(juce::Graphics &g)
{
    if (isLooping)
    {
        sharedIcons->unloopIcon->drawWithin(g, g.getClipBounds().toFloat(), juce::RectanglePlacement::centred, 1.0f);
    }
    else
    {
        sharedIcons->loopIcon->drawWithin(g, g.getClipBounds().toFloat(), juce::RectanglePlacement::centred, 1.0f);
    }
}

void LoopButton::mouseDown(const juce::MouseEvent &me)
{
    auto task = std::make_shared<LoopToggleTask>(!isLooping);
    activityManager.broadcastTask(task);
}