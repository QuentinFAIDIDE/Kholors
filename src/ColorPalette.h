#ifndef DEF_COLOR_PALETTE_HPP
#define DEF_COLOR_PALETTE_HPP

#include <vector>

#include <juce_gui_extra/juce_gui_extra.h>

std::vector<juce::Colour> colourPalette = {
    juce::Colour(static_cast<juce::uint8>(22), static_cast<juce::uint8>(105), static_cast<juce::uint8>(122), static_cast<juce::uint8>(200)),
    juce::Colour(static_cast<juce::uint8>(72), static_cast<juce::uint8>(159), static_cast<juce::uint8>(181), static_cast<juce::uint8>(200)),
    juce::Colour(static_cast<juce::uint8>(130), static_cast<juce::uint8>(192), static_cast<juce::uint8>(204), static_cast<juce::uint8>(200)),
    juce::Colour(static_cast<juce::uint8>(237), static_cast<juce::uint8>(231), static_cast<juce::uint8>(227), static_cast<juce::uint8>(200)),
    juce::Colour(static_cast<juce::uint8>(255), static_cast<juce::uint8>(166), static_cast<juce::uint8>(43), static_cast<juce::uint8>(200))
};

#endif // DEF_COLOR_PALETTE_HPP