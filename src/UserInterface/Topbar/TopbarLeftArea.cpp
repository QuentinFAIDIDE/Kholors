#include "TopbarLeftArea.h"

TopbarLeftArea::TopbarLeftArea() : masterGainVu("Master", VUMETER_ID_MASTER)
{
    addAndMakeVisible(masterGainVu);
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

void TopbarLeftArea::setDataSource(std::shared_ptr<VuMeterDataSource> ds)
{
    masterGainVu.setDataSource(ds);
}

void TopbarLeftArea::resized()
{
    auto rootBounds = getLocalBounds();
    rootBounds.reduce(TOPBAR_SECTIONS_INNER_MARGINS * 2, 0);
    auto masterVuArea = rootBounds.removeFromLeft(VUMETER_WIDGET_WIDTH);

    masterGainVu.setBounds(masterVuArea);
}