#ifndef DEF_CALLBACK_VIEWPORT_HPP
#define DEF_CALLBACK_VIEWPORT_HPP

#include <juce_gui_basics/juce_gui_basics.h>

/**
 * @brief Class that describe an object that listens to juce viewport
 *        movements.
 *
 */
class ViewportScrollListener
{
  public:
    /**
     * @brief called when the viewport we're listening to get scrolled.
     *
     * @param newVisibleArea
     * @param childComponentArea
     */
    virtual void receiveViewportUpdate(const juce::Rectangle<int> &newVisibleArea,
                                       const juce::Rectangle<int> childComponentArea) = 0;
};

/**
 * @brief A viewport inherited from juce viewport, but that can have a listener for
 *        when it is scrolled.
 *
 */
class CallbackViewport : public juce::Viewport
{
  public:
    /**
     * @brief Construct a new CallbackViewport object
     *
     */
    CallbackViewport() : juce::Viewport()
    {
        listener = nullptr;
    }

    /**
     * @brief Called by juce when the visible area achanged, so when a scrolling was made!
     *
     * @param newVisibleArea
     */
    void visibleAreaChanged(const juce::Rectangle<int> &newVisibleArea) override
    {
        if (getViewedComponent() != nullptr && listener != nullptr)
        {
            auto childBounds = getViewedComponent()->getLocalBounds();
            listener->receiveViewportUpdate(newVisibleArea, childBounds);
        }
    }

    /**
     * @brief Record the reference object as the listener for when visible area changed.
     *
     * @param vplistener the new listener that will get its receiveViewportUpdate callback called.
     */
    void setScrollListener(ViewportScrollListener *vplistener)
    {
        listener = vplistener;
    }

  private:
    ViewportScrollListener *listener;
};

#endif // DEF_CALLBACK_VIEWPORT_HPP