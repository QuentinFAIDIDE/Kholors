#include "GitInitRepoDialog.h"

// TODO: move that into main config once we're sure of the color
#define COLOR_DIALOG_BACKGROUND juce::Colour(30, 30, 30)
#define DIALOG_FOOTER_AREA_HEIGHT 48
#define DIALOG_FOOTER_BUTTONS_SPACING 12
#define DIALOG_FOOTER_BUTTONS_WIDTH 90
#define DIALOG_FOOTER_BUTTONS_HEIGHT 28
#define DIALOG_TEXT_ENTRY_HEIGHT 30
#define DIALOG_TEXT_ENTRY_TOP_PADDING 8

#include "../../Config.h"
#include "../Section.h"

GitInitRepoDialog::GitInitRepoDialog()
{
    addAndMakeVisible(closeButton);
    closeButton.setButtonText("Cancel");

    addAndMakeVisible(confirmButton);
    confirmButton.setButtonText("Confirm");

    nameEntry.setCaretVisible(true);
    nameEntry.setScrollbarsShown(false);
    nameEntry.setJustification(juce::Justification::left);
    nameEntry.setDescription("Enter your new track name");
    nameEntry.setMultiLine(false);
    nameEntry.setFont(juce::Font(DEFAULT_FONT_SIZE));
    nameEntry.setMouseCursor(juce::MouseCursor::IBeamCursor);
    nameEntry.setColour(juce::TextEditor::textColourId, COLOR_TEXT_DARKER);
    nameEntry.setColour(juce::TextEditor::ColourIds::backgroundColourId, COLOR_BACKGROUND_LIGHTER.darker(0.1f));
    nameEntry.setColour(juce::TextEditor::outlineColourId, COLOR_BACKGROUND_LIGHTER);
    nameEntry.setColour(juce::TextEditor::focusedOutlineColourId, COLOR_BACKGROUND_LIGHTER.darker(0.05f));
    addAndMakeVisible(nameEntry);

    setSize(GIT_INIT_REPO_DIALOG_WIDTH, GIT_INIT_REPO_DIALOG_HEIGHT);

    closeButton.addListener(this);
    confirmButton.addListener(this);
}

void GitInitRepoDialog::paint(juce::Graphics &g)
{

    g.setColour(COLOR_BACKGROUND);
    g.fillAll();

    auto bounds = getLocalBounds().reduced(4, 4);
    auto bgColor = COLOR_DIALOG_BACKGROUND;
    drawSection(g, bounds, "Pick a name for the new repository:", bgColor);

    auto nameBound = nameEntry.getBounds();
    g.setColour(COLOR_TEXT_DARKER.withAlpha(0.6f));
    auto bottomLine = juce::Line<int>(nameBound.getBottomLeft(), nameBound.getBottomRight());
    g.drawLine(bottomLine.toFloat(), 1.0f);
}

void GitInitRepoDialog::resized()
{
    auto bounds = getLocalBounds();

    auto bottomBand = bounds.removeFromBottom(DIALOG_FOOTER_AREA_HEIGHT);

    // looked better with this little padding
    bounds.removeFromTop(SECTION_TITLE_HEIGHT + DIALOG_TEXT_ENTRY_TOP_PADDING);

    int marginsCenter = (bounds.getHeight() - DIALOG_TEXT_ENTRY_HEIGHT);
    nameEntry.setBounds(bounds.reduced(DIALOG_FOOTER_BUTTONS_SPACING, marginsCenter / 2));

    // remove side margins and center vertically for buttons
    bottomBand.reduce(DIALOG_FOOTER_BUTTONS_SPACING, (DIALOG_FOOTER_AREA_HEIGHT - DIALOG_FOOTER_BUTTONS_HEIGHT) / 2);

    confirmButton.setBounds(bottomBand.removeFromRight(DIALOG_FOOTER_BUTTONS_WIDTH));
    bottomBand.removeFromRight(DIALOG_FOOTER_BUTTONS_SPACING);
    closeButton.setBounds(bottomBand.removeFromRight(DIALOG_FOOTER_BUTTONS_WIDTH));
}

void GitInitRepoDialog::buttonClicked(juce::Button *clickedButton)
{
    if (clickedButton == &closeButton || clickedButton == &confirmButton)
    {
        closeDialog();
    }
}

void GitInitRepoDialog::closeDialog()
{
    if (juce::DialogWindow *dw = findParentComponentOfClass<juce::DialogWindow>())
        dw->exitModalState(0);
}