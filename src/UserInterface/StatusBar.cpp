#include "StatusBar.h"
#include "../Config.h"

StatusBar::StatusBar()
{
    sharedStatusTips->setStatusPainter(this);
}

StatusBar::~StatusBar()
{
    sharedStatusTips->unsetStatusPainter();
}

void StatusBar::paint(juce::Graphics &g)
{
    auto bounds = g.getClipBounds();
    auto font = juce::Font(DEFAULT_FONT_SIZE);
    g.setFont(font);
    g.setColour(COLOR_TEXT);

    std::string versionPlaceholder = "Kholors Alpha v0.0.0";
    bounds.reduce(TAB_PADDING * 3, 0);

    auto versionArea = bounds.removeFromLeft(VERSION_PLACEHOLDER_WIDTH);
    auto actionArea = bounds.removeFromRight(ACTION_PLACEHOLDER_WIDTH);
    auto positionArea = bounds;

    auto lastPositionTipOpt = sharedStatusTips->getPositionStatus();
    if (lastPositionTipOpt.has_value())
    {
        lastPositionTip = *lastPositionTipOpt;
    }

    g.drawText(versionPlaceholder, versionArea, juce::Justification::centredLeft);
    g.drawText(lastPositionTip, positionArea, juce::Justification::centredLeft);
    g.drawText("Ready", actionArea, juce::Justification::centredRight);
}