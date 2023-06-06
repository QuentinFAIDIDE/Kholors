#ifndef DEF_FREQUENCY_GRID_HPP
#define DEF_FREQUENCY_GRID_HPP

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

#define FREQ_GRID_LABEL_HEIGHT 18
#define FREQ_GRID_LABEL_X_PADDING 6
#define FREQ_GRID_LINE_WIDTH 0.35f
#define FREQ_GRID_GRADIENT_WIDTH 60
// how many pixels are full dark on the left side ?
#define FREQ_GRID_GRADIENT_SHIFT 10

class FrequencyGrid : public juce::Component
{
  public:
    FrequencyGrid();

    /**
    paint the frequency grids
    */
    void paint(juce::Graphics &g) override;

  private:
    static std::vector<float> displayedFreqs;
};

#endif // DEF_FREQUENCY_GRID_HPP