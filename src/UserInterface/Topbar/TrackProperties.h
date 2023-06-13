#ifndef DEF_TRACK_PROPERTIES_HPP
#define DEF_TRACK_PROPERTIES_HPP

#include <juce_gui_extra/juce_gui_extra.h>

/**
 Widget for the area to the right of the topbar.
 */
class TrackProperties : public juce::Component
{
  public:
    TrackProperties();
    void paint(juce::Graphics &g) override;
};

#endif // DEF_TRACK_PROPERTIES_HPP