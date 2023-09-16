#ifndef DEF_STATUS_TIPS_HPP
#define DEF_STATUS_TIPS_HPP

#include <mutex>
#include <optional>
#include <string>

#include "juce_gui_basics/juce_gui_basics.h"

// a class responsible for receiving tips from components,
// and exposing them to the status bar.
// Note that it was made to be shared in a static way
// across the soft with a juce SharedResourcePointer.
class StatusTips
{
  public:
    /**
     * @brief Construct a new Status Tips object
     *
     */
    StatusTips();

    /**
     * @brief Set the Position Status string. It is supposed
     *  to be a piece of text that gives info about the cursor position.
     *
     * @param posStatus
     */
    void setPositionStatus(std::string posStatus);

    /**
     * @brief Get the Position Status string. It is supposed to be
     * a piece of text that gives info about the cursor position.
     * This may fail due to lock being not lockable and will return
     * an empty optional.
     *
     * @return std::optional<std::string>
     */
    std::optional<std::string> getPositionStatus();

    /**
     * @brief Set the component that repaint is called on
     * whenever a new status value is set.
     *
     * @param statusPainter a pointer to the juce component that
     * will get its repaint method called
     */
    void setStatusPainter(juce::Component *statusPainter);

    /**
     * @brief Call this before you delete your painter component
     * to reduce risk of bloody segfaults.
     *
     */
    void unsetStatusPainter();

  private:
    std::mutex positionStatusLock; /**< protects the position status string */
    std::string positionStatus;    /**< gives info about current mouse position */

    juce::Component *statusRepaintComponent; /**< The component to repaint when one for the value was found */
};

#endif // DEF_STATUS_TIPS_HPP