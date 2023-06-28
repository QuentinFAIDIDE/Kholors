#include "SampleProperties.h"

#include "../Section.h"
#include "../Widgets/NumericInput.h"
#include <climits>

#define SAMPLEPROPS_MAX_LABEL_WIDTH 60
#define SAMPLEPROPS_INPUT_WIDTH 70

SampleProperties::SampleProperties(ActivityManager &am)
{
    auto fadeInInput = std::make_shared<SampleFadeInput>(true);
    fadeInInput->setUnit("ms");
    am.registerTaskListener(fadeInInput.get());
    fadeInInput->setActivityManager(&am);
    fadeInLine = std::make_shared<LabeledLineContainer>("Fade In:", fadeInInput, SAMPLEPROPS_MAX_LABEL_WIDTH,
                                                        SAMPLEPROPS_INPUT_WIDTH);
    addAndMakeVisible(*fadeInLine);

    std::shared_ptr<GenericNumericInput> fadeOutInput =
        std::make_shared<SampleFadeInput>(false);
    fadeOutInput->setUnit("ms");
    am.registerTaskListener(fadeOutInput.get());
    fadeOutInput->setActivityManager(&am);
    fadeOutLine = std::make_shared<LabeledLineContainer>("Fade Out:", fadeOutInput, SAMPLEPROPS_MAX_LABEL_WIDTH,
                                                         SAMPLEPROPS_INPUT_WIDTH);
    addAndMakeVisible(*fadeOutLine);

    auto gainInput = std::make_shared<SampleGainInput>(true, -12.0, 12.0, 0.1);
    fadeOutInput->setUnit("dB");
    am.registerTaskListener(gainInput.get());
    gainInput->setActivityManager(&am);
    gainLine = std::make_shared<LabeledLineContainer>("Gain:", gainInput, SAMPLEPROPS_MAX_LABEL_WIDTH,
                                                       SAMPLEPROPS_INPUT_WIDTH);
    addAndMakeVisible(*gainLine);
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
    gainLine->setBounds(contentBounds.removeFromTop(LABELED_LINE_CONTAINER_DEFAULT_HEIGHT));
    fadeInLine->setBounds(contentBounds.removeFromTop(LABELED_LINE_CONTAINER_DEFAULT_HEIGHT));
    fadeOutLine->setBounds(contentBounds.removeFromTop(LABELED_LINE_CONTAINER_DEFAULT_HEIGHT));
}