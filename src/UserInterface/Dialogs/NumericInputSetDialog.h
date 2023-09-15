#ifndef DEF_NUMERIC_INPUT_SET_HPP
#define DEF_NUMERIC_INPUT_SET_HPP

#include "../Widgets/NumericInput.h"
#include "LineEntryDialog.h"
#include <exception>
#include <regex>

class NumericInputSetDialog : public LineEntryDialog
{
  public:
    NumericInputSetDialog(ActivityManager &am, NumericInput *numericInputComponent) : LineEntryDialog(am)
    {
        numericInput = numericInputComponent;
        initializeDialog();
    }

    /**
     * @brief      Gets the text entry validation regular expression.
     *
     * @return     A regular expression that passes if text inside text entry component is okay.
     */
    std::regex getEntryRegex()
    {
        return std::basic_regex("^[-]{0,1}[0-9]\\d*(\\.\\d+)?$");
    }

    /**
     * @brief      Gets the text entry description.
     *
     * @return     The text entry description.
     */
    std::string getTextEntryDescription()
    {
        return "Enter the value for your input";
    };

    /**
     * @brief      Gets the dialog width.
     *
     * @return     The dialog width.
     */
    int getDialogWidth()
    {
        return 300;
    }

    /**
     * @brief      Gets the dialog height.
     *
     * @return     The dialog height.
     */
    int getDialogHeight()
    {
        return 80;
    }

    /**
     * @brief      Gets the line of text displayed above text entry
     *             that describe what to enter.
     *
     * @return     The dialog instructions.
     */
    std::string getDialogInstructions()
    {
        return "Enter the value for your input";
    };

    /**
     * @brief      Performs dialog task when user click the confirm button.
     *
     * @param[in]  dialogEntry  The dialog entry text.
     */
    void performDialogTask(std::string dialogEntry)
    {
        try
        {
            float val = std::stof(dialogEntry);
            if (numericInput->isValueValid(val))
            {
                numericInput->emitTaskToSetValue(val);
            }
        }
        catch (std::exception &e)
        {
            std::cerr << "Unable to parse new entered input value: " << e.what() << std::endl;
        }
    }

  private:
    NumericInput *numericInput; /**< Reference to the numeric input this dialog wants to set */
};

#endif // DEF_NUMERIC_INPUT_SET_HPP