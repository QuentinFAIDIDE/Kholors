#include "PlayButton.h"
#include <memory>

PlayButton::PlayButton(ActivityManager &am) : activityManager(am), isPlaying(false)
{
    activityManager.registerTaskListener(this);
}

bool PlayButton::taskHandler(std::shared_ptr<Task> task)
{
    auto playUpdateTask = std::dynamic_pointer_cast<PlayStateUpdateTask>(task);
    if (playUpdateTask != nullptr && playUpdateTask->isCompleted())
    {
        isPlaying = playUpdateTask->isCurrentlyPlaying;
        return false;
    }

    return false;
}

void PlayButton::paint(juce::Graphics &g)
{
    if (isPlaying)
    {
        sharedIcons->pauseIcon->drawWithin(g, g.getClipBounds().toFloat(), juce::RectanglePlacement::centred, 1.0f);
    }
    else
    {
        sharedIcons->playIcon->drawWithin(g, g.getClipBounds().toFloat(), juce::RectanglePlacement::centred, 1.0f);
    }
}

void PlayButton::mouseDown(const juce::MouseEvent &me)
{
    auto task = std::make_shared<PlayStateUpdateTask>(!isPlaying, false);
    activityManager.broadcastTask(task);
}