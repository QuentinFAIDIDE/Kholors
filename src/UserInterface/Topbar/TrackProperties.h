#ifndef DEF_TRACK_PROPERTIES_HPP
#define DEF_TRACK_PROPERTIES_HPP

#include "../Widgets/LabeledLineContainer.h"

#include <juce_gui_extra/juce_gui_extra.h>

/**
 Widget for the area to the right of the topbar.
 */
class TrackProperties : public juce::Component
{
  public:
    TrackProperties();
    void paint(juce::Graphics &g) override;
    void resized() override;

  private:
    std::shared_ptr<LabeledLineContainer> trackTimeInfoLine;
    std::shared_ptr<LabeledLineContainer> trackTempoLine;
};

#endif // DEF_TRACK_PROPERTIES_HPP