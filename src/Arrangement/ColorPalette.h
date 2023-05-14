#ifndef DEF_COLOR_PALETTE_HPP
#define DEF_COLOR_PALETTE_HPP

#include <juce_gui_extra/juce_gui_extra.h>

#include <vector>

// made from https://medialab.github.io/iwanthue/
inline std::vector<juce::Colour> colourPalette = {
    juce::Colour::fromString("ff6cbf73"), juce::Colour::fromString("ffd470e7"), juce::Colour::fromString("ff80c239"),
    juce::Colour::fromString("ffe97bbd"), juce::Colour::fromString("ff40c6b6"), juce::Colour::fromString("ffe09b2e"),
    juce::Colour::fromString("ff7b98ea"), juce::Colour::fromString("ffc4a756"), juce::Colour::fromString("ffb4a1e2"),
    juce::Colour::fromString("ffeb8071")};

#endif // DEF_COLOR_PALETTE_HPP