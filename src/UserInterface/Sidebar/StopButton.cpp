#include "StopButton.h"
#include "PlayButton.h"

StopButton::StopButton(ActivityManager &am) : PlayButton(am)
{
}

void StopButton::paint(juce::Graphics &g)
{
    g.setColour(COLOR_TEXT_DARKER);
    g.fillRect(g.getClipBounds());
}

void StopButton::mouseDown(const juce::MouseEvent &me)
{
    auto task = std::make_shared<PlayStateUpdateTask>(false, true);
    activityManager.broadcastTask(task);
}