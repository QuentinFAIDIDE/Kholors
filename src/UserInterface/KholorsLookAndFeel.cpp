#include "KholorsLookAndFeel.h"

#include "../Config.h"

KholorsLookAndFeel::KholorsLookAndFeel()
{
    setColour(juce::PopupMenu::ColourIds::backgroundColourId, COLOR_BACKGROUND);
    setColour(juce::PopupMenu::ColourIds::highlightedBackgroundColourId, COLOR_BACKGROUND_LIGHTER);
    setColour(juce::TabbedButtonBar::ColourIds::tabOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::TabbedButtonBar::ColourIds::frontOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::TabbedComponent::ColourIds::outlineColourId, juce::Colours::transparentBlack);
}

void KholorsLookAndFeel::drawTabButton(juce::TabBarButton &tb, juce::Graphics &g, bool isMouseOver, bool)
{
    auto bounds = g.getClipBounds();

    g.setColour(COLOR_TEXT);
    g.setFont(juce::Font(DEFAULT_FONT_SIZE));

    if (tb.isFrontTab())
    {
        g.setColour(COLOR_HIGHLIGHT);
    }

    if (isMouseOver)
    {
        g.setColour(COLOR_HIGHLIGHT);
    }

    g.drawText(tb.getButtonText().toUpperCase(), bounds.reduced(4, 0), juce::Justification::centred, true);
}

juce::Font KholorsLookAndFeel::getPopupMenuFont()
{
    return juce::Font(DEFAULT_FONT_SIZE);
}

int KholorsLookAndFeel::getTabButtonBestWidth(juce::TabBarButton &tbb, int)
{
    auto font = juce::Font(DEFAULT_FONT_SIZE);
    int i = tbb.getIndex();
    return font.getStringWidth(tbb.getTabbedButtonBar().getTabNames()[i]) + TAB_BUTTONS_INNER_PADDING;
}

void KholorsLookAndFeel::drawTabbedButtonBarBackground(juce::TabbedButtonBar &, juce::Graphics &g)
{
    g.setColour(COLOR_BACKGROUND);
    g.fillAll();

    g.setColour(COLOR_SEPARATOR_LINE);
    auto line = g.getClipBounds().withHeight(1);
    g.drawRect(line);
}

void KholorsLookAndFeel::drawTabAreaBehindFrontButton(juce::TabbedButtonBar &tb, juce::Graphics &g, int w, int h)
{
    int currentTabIndex = tb.getCurrentTabIndex();
    auto tabBounds = tb.getTabButton(currentTabIndex)->getBounds();
    g.setColour(COLOR_HIGHLIGHT);
    auto line = tabBounds.withY(tabBounds.getY() + tabBounds.getHeight() - TAB_HIGHLIGHT_LINE_WIDTH)
                    .withHeight(TAB_HIGHLIGHT_LINE_WIDTH);
    g.fillRect(line);
}