#include "ConfirmDialog.h"

ConfirmDialog::ConfirmDialog(ActivityManager &am) : activityManager(am)
{
    closeButton.setButtonText("Cancel");
    confirmButton.setButtonText("Confirm");

    addAndMakeVisible(closeButton);
    addAndMakeVisible(confirmButton);

    closeButton.addListener(this);
    confirmButton.addListener(this);
}

void ConfirmDialog::initializeDialog()
{
    setSize(getDialogWidth(), getDialogHeight());
    dialogMessage = getDialogText();
}

void ConfirmDialog::paint(juce::Graphics &g)
{
    g.setColour(COLOR_BACKGROUND);
    g.fillAll();

    auto bounds = getLocalBounds();
    bounds.removeFromBottom(DIALOG_FOOTER_BUTTONS_HEIGHT);
    bounds.reduce(CONFIRM_DIALOG_CONTENT_PADDING, CONFIRM_DIALOG_CONTENT_PADDING);

    g.setColour(COLOR_TEXT_DARKER);
    g.setFont(juce::Font(CONFIRM_DIALOG_TEXT_SIZE));
    // note that unlike usually, we give the coordinates with absolute and not local origin
    g.drawMultiLineText(dialogMessage, getBounds().getX() + bounds.getX(),
                        getBounds().getY() + bounds.getY() + CONFIRM_TEXT_TOP_PADDING, bounds.getWidth(),
                        juce::Justification::centred);
}

void ConfirmDialog::resized()
{
    auto bounds = getLocalBounds();

    auto bottomBand = bounds.removeFromBottom(DIALOG_FOOTER_AREA_HEIGHT);

    // remove side margins and center vertically for buttons
    bottomBand.reduce(DIALOG_FOOTER_BUTTONS_SPACING, (DIALOG_FOOTER_AREA_HEIGHT - DIALOG_FOOTER_BUTTONS_HEIGHT) / 2);

    confirmButton.setBounds(bottomBand.removeFromRight(DIALOG_FOOTER_BUTTONS_WIDTH));
    bottomBand.removeFromRight(DIALOG_FOOTER_BUTTONS_SPACING);
    closeButton.setBounds(bottomBand.removeFromRight(DIALOG_FOOTER_BUTTONS_WIDTH));
}

void ConfirmDialog::closeDialog()
{
    if (juce::DialogWindow *dw = findParentComponentOfClass<juce::DialogWindow>())
        dw->exitModalState(0);
}

void ConfirmDialog::buttonClicked(juce::Button *clickedButton)
{
    if (clickedButton == &closeButton || clickedButton == &confirmButton)
    {
        closeDialog();
    }
    if (clickedButton == &confirmButton)
    {
        performDialogTask();
    }
}

ActivityManager &ConfirmDialog::getActivityManager()
{
    return activityManager;
}
