#include "LayerBar.h"

void LayerBar::paint(juce::Graphics &g)
{
    g.fillAll(COLOR_BACKGROUND_LIGHTER);
    auto bounds = g.getClipBounds();
    juce::Font font(DEFAULT_FONT_SIZE);
    juce::Font plusFont(25);

    auto firstTabWidth = font.getStringWidth("ALL LAYERS") + TAB_BUTTONS_INNER_PADDING;
    auto firstTabArea = bounds.removeFromLeft(firstTabWidth);
    auto secondTabWidth = plusFont.getStringWidth("+") + (TAB_BUTTONS_INNER_PADDING >> 1);

    g.setFont(font);
    g.setColour(COLOR_HIGHLIGHT);
    g.drawText("ALL LAYERS", firstTabArea, juce::Justification::centred);
    g.fillRect(firstTabArea.withY(firstTabArea.getY() + firstTabArea.getHeight() - TAB_HIGHLIGHT_LINE_WIDTH)
                   .withHeight(TAB_HIGHLIGHT_LINE_WIDTH));

    g.setFont(plusFont);
    g.setColour(COLOR_TEXT);
    g.drawText("+", bounds.removeFromLeft(secondTabWidth), juce::Justification::centred);
}

void LayerBar::mouseDown(const juce::MouseEvent &)
{
}