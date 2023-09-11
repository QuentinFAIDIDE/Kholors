#ifndef LAYER_BAR_HPP
#define LAYER_BAR_HPP

#include "juce_gui_basics/juce_gui_basics.h"

#include "../Config.h"

class LayerBar : public juce::Component
{
  public:
    void paint(juce::Graphics &g) override;

    void mouseDown(const juce::MouseEvent &) override;
};

#endif // LAYER_BAR_HPP