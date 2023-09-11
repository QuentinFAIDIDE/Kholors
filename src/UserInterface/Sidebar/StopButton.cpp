#include "StopButton.h"
#include "PlayButton.h"

StopButton::StopButton(ActivityManager &am) : PlayButton(am)
{
}

void StopButton::paint(juce::Graphics &g)
{
    g.setColour(COLOR_TEXT_DARKER);

    // we make a square that fits inside the drawable area
    auto bounds = getLocalBounds();
    if (bounds.getWidth() < bounds.getHeight())
    {
        bounds.reduce(0, (bounds.getHeight() - bounds.getWidth()) >> 1);
    }
    else if (bounds.getWidth() > bounds.getHeight())
    {
        bounds.reduce((bounds.getWidth() - bounds.getHeight()) >> 1, 0);
    }

    g.fillRect(bounds);
}

void StopButton::mouseDown(const juce::MouseEvent &me)
{
    auto task = std::make_shared<PlayStateUpdateTask>(false, true);
    activityManager.broadcastTask(task);
}