#ifndef DEF_TEMPO_GRID_HPP
#define DEF_TEMPO_GRID_HPP

#include <juce_gui_basics/juce_gui_basics.h>

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
};

#endif // DEF_TEMPO_GRID_HPP