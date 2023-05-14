#ifndef DEF_SECTION_HPP
#define DEF_SECTION_HPP

#include <juce_gui_extra/juce_gui_extra.h>

#define SECTION_TITLE_HEIGHT 24

void drawSection(juce::Graphics &g, juce::Rectangle<int> &bounds, juce::String title, juce::Colour &background);

#endif // DEF_SECTION_HPP