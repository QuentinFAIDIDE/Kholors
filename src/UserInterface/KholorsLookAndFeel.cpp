#include "KholorsLookAndFeel.h"

#include "../Config.h"

KholorsLookAndFeel::KholorsLookAndFeel()
{
    setColour(juce::PopupMenu::ColourIds::backgroundColourId, COLOR_BACKGROUND);
    setColour(juce::PopupMenu::ColourIds::highlightedBackgroundColourId, COLOR_SELECTED_BACKGROUND);

    setColour(juce::TabbedButtonBar::ColourIds::tabOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::TabbedButtonBar::ColourIds::frontOutlineColourId, juce::Colours::transparentBlack);

    setColour(juce::TabbedComponent::ColourIds::outlineColourId, juce::Colours::transparentBlack);

    setColour(juce::ScrollBar::ColourIds::thumbColourId, COLOR_HIGHLIGHT);

    setColour(juce::CaretComponent::ColourIds::caretColourId, COLOR_HIGHLIGHT);

    setColour(juce::TreeView::ColourIds::linesColourId, COLOR_TEXT.withAlpha(0.8f));
    setColour(juce::TreeView::ColourIds::selectedItemBackgroundColourId, COLOR_SELECTED_BACKGROUND);

    setColour(juce::ListBox::ColourIds::backgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::ListBox::ColourIds::outlineColourId, juce::Colours::transparentBlack);

    setColour(juce::TextEditor::ColourIds::textColourId, COLOR_TEXT);
    setColour(juce::TextEditor::ColourIds::backgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::TextEditor::ColourIds::outlineColourId, juce::Colours::transparentBlack);
    setColour(juce::TextEditor::ColourIds::focusedOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::TextEditor::ColourIds::highlightColourId, COLOR_HIGHLIGHT);
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