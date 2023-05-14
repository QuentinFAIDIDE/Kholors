#include "KholorsLookAndFeel.h"

#include "../Config.h"

KholorsLookAndFeel::KholorsLookAndFeel()
{
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