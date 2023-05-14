#include "TopbarRightArea.h"

#include "../../Arrangement/ActivityManager.h"
#include "ColorPicker.h"

TopbarRightArea::TopbarRightArea(ActivityManager &am) : colorPicker(am)
{
    addAndMakeVisible(colorPicker);
    colorPicker.setVisible(true);
    colorPicker.setSize(96, 200);
}

void TopbarRightArea::paint(juce::Graphics &g)
{
    auto constBounds = g.getClipBounds();
    bounds = constBounds.reduced(TOPBAR_SECTIONS_INNER_MARGINS / 2, TOPBAR_SECTIONS_INNER_MARGINS).toFloat();

    auto leftLine = juce::Line<float>(bounds.getTopLeft(), bounds.getBottomLeft());
    auto rightLine = juce::Line<float>(bounds.getTopRight(), bounds.getBottomRight());

    g.setColour(COLOR_LABELS_BORDER.withAlpha(0.5f));
    g.drawLine(leftLine, 0.5);
    g.drawLine(rightLine, 0.5);
}

void TopbarRightArea::resized()
{
    auto colorPickerBounds =
        getLocalBounds().reduced(TOPBAR_SECTIONS_INNER_MARGINS * 2, 0).removeFromRight(COLORPICKER_WIDTH);
    colorPicker.setBounds(colorPickerBounds);
}