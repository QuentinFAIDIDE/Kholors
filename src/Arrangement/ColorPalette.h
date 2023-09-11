#ifndef DEF_COLOR_PALETTE_HPP
#define DEF_COLOR_PALETTE_HPP

#include <juce_gui_extra/juce_gui_extra.h>

#include <vector>

// made from https://medialab.github.io/iwanthue/
inline std::vector<juce::Colour> colourPalette = {
    juce::Colour::fromString("ff399898"), juce::Colour::fromString("ff3c7a98"), juce::Colour::fromString("ff4056a5"),
    juce::Colour::fromString("ff663fc6"), juce::Colour::fromString("ff8c2fd3"), juce::Colour::fromString("ffcc34c9"),
    juce::Colour::fromString("ffc2448a"), juce::Colour::fromString("ffc64264"), juce::Colour::fromString("ff9b5f31"),
    juce::Colour::fromString("ffc5a536"), juce::Colour::fromString("ffbec83d"), juce::Colour::fromString("ff97c43e"),
    juce::Colour::fromString("ff49c479")};

#endif // DEF_COLOR_PALETTE_HPP