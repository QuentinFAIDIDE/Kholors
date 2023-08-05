#include "KholorsLookAndFeel.h"

#include "../Config.h"

#define POPUP_MENU_IDEAL_HEIGHT 18;
#define POPUP_MENU_SEPARATOR_IDEAL_HEIGHT 4;

KholorsLookAndFeel::KholorsLookAndFeel()
{
    setColour(juce::PopupMenu::ColourIds::backgroundColourId, COLOR_BACKGROUND_HIGHLIGHT);
    setColour(juce::PopupMenu::ColourIds::highlightedBackgroundColourId, COLOR_BACKGROUND_HIGHLIGHT.brighter(0.2f));
}

void KholorsLookAndFeel::drawTabButton(juce::TabBarButton &tb, juce::Graphics &g, bool isMouseOver, bool isMouseDown)
{
    auto bounds = g.getClipBounds();

    g.setColour(COLOR_TEXT_DARKER.withAlpha(0.5f));
    g.drawRect(bounds.reduced(0).withTrimmedBottom(0), 1);

    if (tb.isFrontTab())
    {
        g.setColour(COLOR_TEXT_DARKER);
    }

    g.drawText(tb.getButtonText(), bounds.reduced(4, 0), juce::Justification::centred, true);
}

juce::Font KholorsLookAndFeel::getPopupMenuFont()
{
    return juce::Font(DEFAULT_FONT_SIZE);
}