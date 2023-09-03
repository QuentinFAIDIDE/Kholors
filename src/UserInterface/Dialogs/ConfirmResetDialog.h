#ifndef CONFIRM_RESET_DIALOG_HPP
#define CONFIRM_RESET_DIALOG_HPP

#include "ConfirmDialog.h"
#include <memory>

#define CONFIRM_RESET_DIALOG_WIDTH 400
#define CONFIRM_RESET_DIALOG_HEIGHT 100

class ConfirmResetDialog : public ConfirmDialog
{
  public:
    ConfirmResetDialog(ActivityManager &am) : ConfirmDialog(am)
    {
        initializeDialog();
    }

    /**
     * @brief      Return the text that is displayed in the dialog window.
     *
     * @return     The dialog text.
     */
    std::string getDialogText()
    {
        return std::string("Are you sure you want permanently discard changes performed since last commit ?");
    }

    /**
     * @brief      Gets the dialog width.
     *
     * @return     The dialog width.
     */
    int getDialogWidth()
    {
        return CONFIRM_RESET_DIALOG_WIDTH;
    }

    /**
     * @brief      Gets the dialog height.
     *
     * @return     The dialog height.
     */
    int getDialogHeight()
    {
        return CONFIRM_RESET_DIALOG_HEIGHT;
    }

    /**
     * @brief      Performs dialog task when user click the confirm button.
     *
     */
    void performDialogTask()
    {
        auto resetHeadTask = std::make_shared<GitHeadResetTask>();
        getActivityManager().broadcastTask(resetHeadTask);
    }
};

#endif // CONFIRM_RESET_DIALOG_HPP