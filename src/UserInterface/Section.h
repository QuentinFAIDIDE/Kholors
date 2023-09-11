#ifndef DEF_SECTION_HPP
#define DEF_SECTION_HPP

#include <juce_gui_extra/juce_gui_extra.h>

#define SECTION_TITLE_HEIGHT 24
#define SECTION_TITLE_HEIGHT_SMALL 16

void drawSection(juce::Graphics &g, juce::Rectangle<int> &bounds, juce::String title);

#endif // DEF_SECTION_HPP