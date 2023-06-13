#ifndef DEF_LABELED_LINE_CONTAINER_HPP
#define DEF_LABELED_LINE_CONTAINER_HPP

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

#define LABELED_LINE_CONTAINER_DEFAULT_HEIGHT 25
#define LABELED_LINE_CONTAINER_PADDING 2

class LabeledLineContainer : public juce::Component
{
  public:
    LabeledLineContainer(std::string label, std::shared_ptr<juce::Component> content, int maxLabelWidth,
                         int contentWidth);
    std::shared_ptr<juce::Component> getContent();
    void paint(juce::Graphics &) override;
    void resized() override;

  private:
    std::shared_ptr<juce::Component> content;
    std::string label;
    int maxLabelWidth;
    int contentWidth;
    juce::Rectangle<int> labelLocalBounds;
    juce::Rectangle<int> contentLocalBounds;
};

#endif