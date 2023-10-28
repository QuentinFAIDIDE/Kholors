#ifndef DEF_TEMPO_GRID_HPP
#define DEF_TEMPO_GRID_HPP

#include "../../Arrangement/ActivityManager.h"
#include "../IconsLoader.h"
#include "../ViewPosition.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

#define LOOP_SECTION_LINE_WIDTH 10
#define COLOR_LOOP_SECTION juce::Colour(150, 150, 200)
#define LOOP_SECTION_BORDERS_RECT_WIDTH 25
#define LOOP_SECTION_BORDERS_RECT_HEIGHT 40

/**
 Component that draw a tempo grid in the middle
 of the view with bar counts.
 */
class TempoGrid : public juce::Component, public TaskListener, public ViewPositionListener
{
  public:
    TempoGrid(ActivityManager &am);

    /**
    paint the grid over the rest meter
    */
    void paint(juce::Graphics &g) override;

    /**
     Updates the track tempo.
     */
    void updateTempo(float);

    /**
     * @brief      Test whether the click should be handled
     *             by this component or passed to the parent (arrangement area).
     *
     * @param[in]  x     x pos relative to left edge of component
     * @param[in]  y     y pos relative to top edge of component
     *
     * @return     true if click is inside, false otherwise
     */
    bool hitTest(int x, int y) override;

    /**
     * @brief      Handle task broadcasted by activity manager.
     *             returned value is true if we stop broadcasting
     *             the task further. Generally we want to stop it
     *             if no other service may need to catch it.
     *             So we tend let "completed" task pass to potentially
     *             other listening components, and stop the one
     *             we performed, while eventually broadcasting
     *             the completed version of the task.
     */
    bool taskHandler(std::shared_ptr<Task>) override;

    /**
     * @brief      Called by juce when the mouse is clicked.
     *
     */
    void mouseDown(const juce::MouseEvent &me) override;

    /**
     * @brief      Called by juce when the mouse button is released.
     *
     */
    void mouseUp(const juce::MouseEvent &me) override;

    /**
     * @brief      Called by juce when the mouse button is down and
     *             there's a movement.
     *
     */
    void mouseDrag(const juce::MouseEvent &me) override;

    /**
     * @brief      Called by juce when the mouse is moved
     *
     */
    void mouseMove(const juce::MouseEvent &me) override;

    /**
     * @brief Called when the view position in audio frames is updated.
     *
     * @param int the position of the view in audio frames (samples).
     */
    void viewPositionUpdateCallback() override;

  private:
    // track tempo
    float tempo;

    // true if the left handle of the loop is dragged, false if right
    bool draggingLeftHandle;

    // is the loop mode toggled ?
    bool loopModeToggle;

    // loop section position in audio frames
    int64_t loopSectionStartFrame, loopSectionStopFrame;

    ActivityManager &activityManager;

    /**
     * The task that will be broadcasted once the drag
     * of the loop section is completed. Mostly
     * usefull for storing the loop initial position.
     */
    std::shared_ptr<LoopMovingTask> currentDragTask;

    /**
     * Shared svg icons.
     */
    juce::SharedResourcePointer<IconsLoader> sharedIcons;

    /**
     * Object where to read position updates from
     */
    juce::SharedResourcePointer<ViewPosition> viewPositionManager;

    ///////////////////////////////

    /**
     * @brief      Paint gradient in the middle.
     *
     * @param      g           juce graphics context
     * @param[in]  halfBounds  The bounds of half of the entire area
     */
    void paintMiddleGradient(juce::Graphics &g, juce::Rectangle<int> halfBounds);

    /**
     * @brief      Paints the ticks in the middle of the screen.
     *
     * @param      g           juce graphic context
     * @param[in]  bounds      The bounds of the entire area
     * @param[in]  halfBounds  The bounds of half of the entire area
     */
    void paintTicks(juce::Graphics &g, juce::Rectangle<int> bounds, juce::Rectangle<int> halfBounds);

    /**
     * @brief      Paint the loop section.
     */
    void paintLoopSection(juce::Graphics &g);

    /**
     * @brief      Paint the loop borders and line.
     *
     * @param      g                             Juce graphics context.
     * @param[in]  loopStartScreenPosProportion  The proportition of the screen width at which the beginning
     *                                           of the loop section lies.
     * @param[in]  loopStopScreenPosProportion   The proportition of the screen width at which the end
     *                                           of the loop section lies.
     */
    void paintLoopBorder(juce::Graphics &g, float loopStartScreenPosProportion, float loopStopScreenPosProportion);

    /**
     * @brief      Paint the whole screen outside the loop
     *             with an opaque colored outline.
     *
     * @param      g                             Juce graphics context.
     * @param[in]  loopStartScreenPosProportion  The proportition of the screen width at which the beginning
     *                                           of the loop section lies.
     * @param[in]  loopStopScreenPosProportion   The proportition of the screen width at which the end
     *                                           of the loop section lies.
     */
    void paintColoredLoopOutline(juce::Graphics &g, float loopStartScreenPosProportion,
                                 float loopStopScreenPosProportion);

    /**
     * @brief      Return the area of the left or right handle depending on the bool.
     *             It return position in the local component coordinates pixel system.
     *
     * @param[in]  left  if true return left handle area, if false, right one.
     *
     * @return     The loop handle area.
     */
    juce::Rectangle<int> getLoopHandleArea(bool left);
};

#endif // DEF_TEMPO_GRID_HPP