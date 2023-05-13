#include "TopbarRightArea.h"

TopbarRightArea::TopbarRightArea()
{
}

void TopbarRightArea::paint(juce::Graphics &g)
{
    auto constBounds = g.getClipBounds();
    bounds = constBounds.reduced(TOPBAR_SECTIONS_INNER_MARGINS, TOPBAR_SECTIONS_INNER_MARGINS).toFloat();

    auto leftLine = juce::Line<float>(bounds.getTopLeft(), bounds.getBottomLeft());
    auto rightLine = juce::Line<float>(bounds.getTopRight(), bounds.getBottomRight());

    g.setColour(COLOR_LABELS_BORDER);
    g.drawLine(leftLine);
    g.drawLine(rightLine);
}

void TopbarRightArea::resized()
{
}