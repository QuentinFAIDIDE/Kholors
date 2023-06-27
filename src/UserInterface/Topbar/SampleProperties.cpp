#include "SampleProperties.h"

#include "../Section.h"
#include "../Widgets/NumericInput.h"
#include <climits>

#define SAMPLEPROPS_MAX_LABEL_WIDTH 60
#define SAMPLEPROPS_INPUT_WIDTH 70

SampleProperties::SampleProperties(ActivityManager &am)
{
    std::shared_ptr<GenericNumericInput> fadeInInput =
        std::make_shared<GenericNumericInput>(true, 0, SAMPLEPLAYER_MAX_FADE_MS, 1);
    fadeInInput->setUnit("ms");
    am.registerTaskListener(fadeInInput.get());
    fadeInInput->setActivityManager(&am);
    fadeInLine = std::make_shared<LabeledLineContainer>("Fade In:", fadeInInput, SAMPLEPROPS_MAX_LABEL_WIDTH,
                                                        SAMPLEPROPS_INPUT_WIDTH);
    addAndMakeVisible(*fadeInLine);

    std::shared_ptr<GenericNumericInput> fadeOutInput =
        std::make_shared<GenericNumericInput>(true, 0, SAMPLEPLAYER_MAX_FADE_MS, 1);
    fadeOutInput->setUnit("ms");
    am.registerTaskListener(fadeOutInput.get());
    fadeOutInput->setActivityManager(&am);
    fadeOutLine = std::make_shared<LabeledLineContainer>("Fade In:", fadeOutInput, SAMPLEPROPS_MAX_LABEL_WIDTH,
                                                         SAMPLEPROPS_INPUT_WIDTH);
    addAndMakeVisible(*fadeOutLine);

    std::shared_ptr<GenericNumericInput> groupIdInput = std::make_shared<GenericNumericInput>(true, 0, INT_MAX, 1);
    am.registerTaskListener(groupIdInput.get());
    groupIdInput->setActivityManager(&am);
    groupLine = std::make_shared<LabeledLineContainer>("Group Id:", groupIdInput, SAMPLEPROPS_MAX_LABEL_WIDTH,
                                                       SAMPLEPROPS_INPUT_WIDTH);
    addAndMakeVisible(*groupLine);
}

void SampleProperties::paint(juce::Graphics &g)
{
    auto bounds = g.getClipBounds();

    juce::Colour bg = juce::Colours::transparentBlack;
    drawSection(g, bounds, "Selected Sample Properties", bg, true);
}

void SampleProperties::resized()
{
    auto contentBounds = getLocalBounds();
    contentBounds.removeFromTop(SECTION_TITLE_HEIGHT_SMALL);
    fadeInLine->setBounds(contentBounds.removeFromTop(LABELED_LINE_CONTAINER_DEFAULT_HEIGHT));
    fadeOutLine->setBounds(contentBounds.removeFromTop(LABELED_LINE_CONTAINER_DEFAULT_HEIGHT));
    groupLine->setBounds(contentBounds.removeFromTop(LABELED_LINE_CONTAINER_DEFAULT_HEIGHT));
}