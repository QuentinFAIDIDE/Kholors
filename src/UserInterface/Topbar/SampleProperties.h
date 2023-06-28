#ifndef DEF_SAMPLE_PROPERTIES_HPP
#define DEF_SAMPLE_PROPERTIES_HPP

#include "../../Arrangement/ActivityManager.h"
#include "../Widgets/LabeledLineContainer.h"
#include <juce_gui_extra/juce_gui_extra.h>
#include <memory>

class SampleProperties : public juce::Component, public TaskListener
{
  public:
    SampleProperties(ActivityManager &am);
    void paint(juce::Graphics &g) override;
    void resized() override;
    bool taskHandler(std::shared_ptr<Task> task) override;

  private:
    std::shared_ptr<LabeledLineContainer> gainLine;
    std::shared_ptr<LabeledLineContainer> fadeInLine;
    std::shared_ptr<LabeledLineContainer> fadeOutLine;
};

#endif // DEF_SAMPLE_PROPERTIES_HPP