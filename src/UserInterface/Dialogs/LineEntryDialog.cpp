#include "LineEntryDialog.h"

#include <memory>
#include <regex>

#include "../../Config.h"

LineEntryDialog::LineEntryDialog(ActivityManager &am) : contentValidationRegex(".*"), activityManager(am)
{

    addAndMakeVisible(closeButton);
    closeButton.setButtonText("Cancel");

    addAndMakeVisible(confirmButton);
    confirmButton.setButtonText("Confirm");
    confirmButton.setEnabled(false);

    textEntry.setCaretVisible(true);
    textEntry.setScrollbarsShown(false);
    textEntry.setJustification(juce::Justification::left);
    textEntry.setMultiLine(false);
    textEntry.setFont(juce::Font(DEFAULT_FONT_SIZE));
    textEntry.setMouseCursor(juce::MouseCursor::IBeamCursor);
    textEntry.setColour(juce::TextEditor::textColourId, COLOR_TEXT_DARKER);
    textEntry.setColour(juce::TextEditor::ColourIds::backgroundColourId, COLOR_BACKGROUND_LIGHTER.darker(0.1f));
    textEntry.setColour(juce::TextEditor::outlineColourId, COLOR_BACKGROUND_LIGHTER);
    textEntry.setColour(juce::TextEditor::focusedOutlineColourId, COLOR_BACKGROUND_LIGHTER.darker(0.05f));
    addAndMakeVisible(textEntry);

    closeButton.addListener(this);
    confirmButton.addListener(this);
    textEntry.addListener(this);
}

void LineEntryDialog::initializeDialog()
{
    contentValidationRegex = this->getEntryRegex();
    setSize(getDialogWidth(), getDialogHeight());
    textEntry.setDescription(getTextEntryDescription());
}

void LineEntryDialog::paint(juce::Graphics &g)
{

    g.setColour(COLOR_BACKGROUND);
    g.fillAll();

    auto bounds = getLocalBounds().reduced(4, 4);
    drawSection(g, bounds, getDialogInstructions() + ":");

    auto nameBound = textEntry.getBounds();
    g.setColour(COLOR_TEXT_DARKER.withAlpha(0.6f));
    auto bottomLine = juce::Line<int>(nameBound.getBottomLeft(), nameBound.getBottomRight());
    g.drawLine(bottomLine.toFloat(), 1.0f);
}

void LineEntryDialog::resized()
{
    auto bounds = getLocalBounds();

    auto bottomBand = bounds.removeFromBottom(DIALOG_FOOTER_AREA_HEIGHT);

    // looked better with this little padding
    bounds.removeFromTop(SECTION_TITLE_HEIGHT + DIALOG_TEXT_ENTRY_TOP_PADDING);

    int marginsCenter = (bounds.getHeight() - DIALOG_TEXT_ENTRY_HEIGHT);
    textEntry.setBounds(bounds.reduced(DIALOG_FOOTER_BUTTONS_SPACING, marginsCenter / 2));

    // remove side margins and center vertically for buttons
    bottomBand.reduce(DIALOG_FOOTER_BUTTONS_SPACING, (DIALOG_FOOTER_AREA_HEIGHT - DIALOG_FOOTER_BUTTONS_HEIGHT) / 2);

    confirmButton.setBounds(bottomBand.removeFromRight(DIALOG_FOOTER_BUTTONS_WIDTH));
    bottomBand.removeFromRight(DIALOG_FOOTER_BUTTONS_SPACING);
    closeButton.setBounds(bottomBand.removeFromRight(DIALOG_FOOTER_BUTTONS_WIDTH));
}

void LineEntryDialog::buttonClicked(juce::Button *clickedButton)
{
    if (clickedButton == &closeButton || clickedButton == &confirmButton)
    {
        closeDialog();
    }
    if (clickedButton == &confirmButton)
    {
        performDialogTask(textEntryMessage);
    }
}

void LineEntryDialog::closeDialog()
{
    if (juce::DialogWindow *dw = findParentComponentOfClass<juce::DialogWindow>())
        dw->exitModalState(0);
}

/** Called when the user changes the text in some way. */
void LineEntryDialog::textEditorTextChanged(juce::TextEditor &te)
{
    textEntryMessage = te.getText().toStdString();
    confirmButton.setEnabled(nameIsValid());
}

/** Called when the user presses the return key. */
void LineEntryDialog::textEditorReturnKeyPressed(juce::TextEditor &)
{
    if (nameIsValid())
    {
        closeDialog();
        performDialogTask(textEntryMessage);
    }
}

bool LineEntryDialog::nameIsValid()
{
    return std::regex_match(textEntryMessage, contentValidationRegex);
}

ActivityManager &LineEntryDialog::getActivityManager()
{
    return activityManager;
}