#include "LabeledLineContainer.h"

#include "../../Config.h"

LabeledLineContainer::LabeledLineContainer(std::string lab, std::shared_ptr<juce::Component> widget, int maxLabWidth,
                                           int contentWidthDesired)
    : content(widget), label(lab), maxLabelWidth(maxLabWidth), contentWidth(contentWidthDesired)
{
    addAndMakeVisible(*content);
}

std::shared_ptr<juce::Component> LabeledLineContainer::getContent()
{
    return content;
}

void LabeledLineContainer::paint(juce::Graphics &g)
{
    g.setFont(juce::Font(SMALLER_FONT_SIZE));
    g.setColour(COLOR_TEXT_DARKER);
    g.drawText(label, labelLocalBounds, juce::Justification::centredLeft, true);
}

void LabeledLineContainer::resized()
{
    auto bounds = getLocalBounds().reduced(LABELED_LINE_CONTAINER_PADDING);

    contentLocalBounds = bounds.removeFromRight(contentWidth);
    labelLocalBounds = bounds;

    // maximum value for the
    if (labelLocalBounds.getWidth() > maxLabelWidth)
    {
        labelLocalBounds = labelLocalBounds.removeFromLeft(maxLabelWidth);
    }

    content->setBounds(contentLocalBounds);
}