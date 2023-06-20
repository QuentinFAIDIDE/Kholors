#ifndef DEF_TEMPO_GRID_HPP
#define DEF_TEMPO_GRID_HPP

#include <juce_gui_basics/juce_gui_basics.h>

#define LOOP_SECTION_LINE_WIDTH 4
#define COLOR_LOOP_SECTION juce::Colour(150, 150, 200)
#define LOOP_SECTION_BORDERS_RECT_WIDTH 25
#define LOOP_SECTION_BORDERS_RECT_HEIGHT 40

/**
 Component that draw a tempo grid in the middle
 of the view with bar counts.
 */
class TempoGrid : public juce::Component
{
  public:
    TempoGrid();

    /**
    paint the grid over the rest meter
    */
    void paint(juce::Graphics &g) override;

    /**
     Updates the track tempo.
     */
    void updateTempo(float);

    /**
     Update position of view
     */
    void updateView(int viewPosition, float viewScale);

  private:
    // the position in the track in audio frames
    int viewPosition;
    // the scale in frames per pixels
    float viewScale;

    // track tempo
    float tempo;

    // is the loop mode toggled ?
    bool loopModeToggle;

    // loop section position in audio frames
    int64_t loopSectionStartFrame, loopSectionStopFrame;

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
};

#endif // DEF_TEMPO_GRID_HPP