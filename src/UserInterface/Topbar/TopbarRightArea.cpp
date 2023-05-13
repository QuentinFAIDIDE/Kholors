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

    g.setColour(COLOR_LABELS_BORDER.withAlpha(0.5f));
    g.drawLine(leftLine, 0.5);
    g.drawLine(rightLine, 0.5);
}

void TopbarRightArea::resized()
{
}