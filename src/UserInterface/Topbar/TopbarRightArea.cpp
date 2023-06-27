#include "TopbarRightArea.h"

#include "../../Arrangement/ActivityManager.h"
#include "ColorPicker.h"

TopbarRightArea::TopbarRightArea(ActivityManager &am)
    : colorPicker(am), selectionGainVu("Selected", VUMETER_ID_SELECTED), sampleProperties(am)
{
    addAndMakeVisible(colorPicker);
    colorPicker.setVisible(true);
    colorPicker.setSize(96, 200);

    addAndMakeVisible(selectionGainVu);

    addAndMakeVisible(sampleProperties);
}

void TopbarRightArea::setDataSource(std::shared_ptr<VuMeterDataSource> ds)
{
    selectionGainVu.setDataSource(ds);
}

void TopbarRightArea::paint(juce::Graphics &g)
{
    auto constBounds = g.getClipBounds();
    bounds = constBounds.reduced(TOPBAR_SECTIONS_INNER_MARGINS, TOPBAR_SECTIONS_INNER_MARGINS).toFloat();

    // draw a separator to the left
    auto leftLine = juce::Line<float>(bounds.getTopLeft(), bounds.getBottomLeft());
    g.setColour(COLOR_LABELS_BORDER.withAlpha(0.5f));
    g.drawLine(leftLine, 0.5);
}

void TopbarRightArea::resized()
{
    auto colorPickerBounds = getLocalBounds().removeFromRight(COLORPICKER_WIDTH);
    colorPicker.setBounds(colorPickerBounds);

    // bounds of the vu meter widget that display volume for selected samples.
    // Note that VUMETER_WIDTH does not include side margins so we double it to make room.
    auto selectedGainVuBounds = colorPickerBounds.withWidth(VUMETER_WIDGET_WIDTH);
    selectedGainVuBounds.setX(selectedGainVuBounds.getX() - selectedGainVuBounds.getWidth());
    selectionGainVu.setBounds(selectedGainVuBounds);

    auto samplePropsBounds = getLocalBounds();
    samplePropsBounds.removeFromRight(COLORPICKER_WIDTH);
    samplePropsBounds.removeFromRight(VUMETER_WIDGET_WIDTH);
    sampleProperties.setBounds(samplePropsBounds);
}