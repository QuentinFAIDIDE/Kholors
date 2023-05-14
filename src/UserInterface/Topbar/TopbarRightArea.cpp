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
    bounds = constBounds.toFloat();

    // draw a separator to the left
    auto leftLine = juce::Line<float>(bounds.getTopLeft(), bounds.getBottomLeft());
    g.setColour(COLOR_LABELS_BORDER.withAlpha(0.5f));
    g.drawLine(leftLine, 0.5);
}

void TopbarRightArea::resized()
{
    auto colorPickerBounds = getLocalBounds().removeFromRight(COLORPICKER_WIDTH);
    colorPicker.setBounds(colorPickerBounds);
}