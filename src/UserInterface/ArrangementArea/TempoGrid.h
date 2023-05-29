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
};

#endif // DEF_TEMPO_GRID_HPP