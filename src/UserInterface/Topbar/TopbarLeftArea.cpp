#include "TopbarLeftArea.h"

TopbarLeftArea::TopbarLeftArea() : masterGainVu("Master", "master"), inputGainVu("Input", "master")
{
    addAndMakeVisible(masterGainVu);
    addAndMakeVisible(inputGainVu);
}

void TopbarLeftArea::paint(juce::Graphics &g)
{
    auto constBounds = g.getClipBounds();
    bounds = constBounds.reduced(TOPBAR_SECTIONS_INNER_MARGINS, TOPBAR_SECTIONS_INNER_MARGINS).toFloat();

    auto leftLine = juce::Line<float>(bounds.getTopLeft(), bounds.getBottomLeft());
    auto rightLine = juce::Line<float>(bounds.getTopRight(), bounds.getBottomRight());

    g.setColour(COLOR_LABELS_BORDER.withAlpha(0.5f));
    g.drawLine(leftLine, 0.5);
    g.drawLine(rightLine, 0.5);
}

void TopbarLeftArea::resized()
{
    auto rootBounds = getLocalBounds();
    rootBounds.reduce(TOPBAR_SECTIONS_INNER_MARGINS * 2, 0);
    auto masterVuArea = rootBounds.removeFromLeft(VUMETER_WIDGET_WIDTH);
    auto inputVuArea = rootBounds.removeFromLeft(VUMETER_WIDGET_WIDTH);

    masterGainVu.setBounds(masterVuArea);
    inputGainVu.setBounds(inputVuArea);
}