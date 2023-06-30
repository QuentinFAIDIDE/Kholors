#include "SampleProperties.h"

#include "../Section.h"
#include "SampleInputs.h"
#include <climits>
#include <memory>

#define SAMPLEPROPS_MAX_LABEL_WIDTH 60
#define SAMPLEPROPS_INPUT_WIDTH 70

SampleProperties::SampleProperties(ActivityManager &am)
{
    auto fadeInInput = std::make_shared<SampleFadeInput>(true);
    fadeInInput->setUnit("ms");
    am.registerTaskListener(fadeInInput.get());
    fadeInInput->setActivityManager(&am);
    fadeInInput->setMinDragUpdate(1.0f);
    fadeInLine = std::make_shared<LabeledLineContainer>("Fade In:", fadeInInput, SAMPLEPROPS_MAX_LABEL_WIDTH,
                                                        SAMPLEPROPS_INPUT_WIDTH);
    addAndMakeVisible(*fadeInLine);

    auto fadeOutInput = std::make_shared<SampleFadeInput>(false);
    fadeOutInput->setUnit("ms");
    am.registerTaskListener(fadeOutInput.get());
    fadeOutInput->setActivityManager(&am);
    fadeOutInput->setMinDragUpdate(1.0f);
    fadeOutLine = std::make_shared<LabeledLineContainer>("Fade Out:", fadeOutInput, SAMPLEPROPS_MAX_LABEL_WIDTH,
                                                         SAMPLEPROPS_INPUT_WIDTH);
    addAndMakeVisible(*fadeOutLine);

    auto gainInput = std::make_shared<SampleGainInput>();
    gainInput->setUnit("dB");
    am.registerTaskListener(gainInput.get());
    gainInput->setActivityManager(&am);
    gainLine = std::make_shared<LabeledLineContainer>("Gain:", gainInput, SAMPLEPROPS_MAX_LABEL_WIDTH,
                                                      SAMPLEPROPS_INPUT_WIDTH);
    addAndMakeVisible(*gainLine);

    am.registerTaskListener(this);
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

bool SampleProperties::taskHandler(std::shared_ptr<Task> task)
{
    auto selectionUpdateTask = std::dynamic_pointer_cast<SelectionChangingTask>(task);
    if (selectionUpdateTask != nullptr && selectionUpdateTask->isCompleted())
    {
        auto fadeInInput = std::dynamic_pointer_cast<SampleFadeInput>(fadeInLine->getContent());
        auto fadeOutInput = std::dynamic_pointer_cast<SampleFadeInput>(fadeOutLine->getContent());

        if (fadeInInput == nullptr || fadeOutInput == nullptr)
        {
            return false;
        }

        if (selectionUpdateTask->newSelectedTracks.size() == 1)
        {
            int firstSampleId = *selectionUpdateTask->newSelectedTracks.begin();
            fadeInInput->setSampleId(firstSampleId);
            fadeOutInput->setSampleId(firstSampleId);
        }
        else
        {
            fadeInInput->setSampleId(-1);
            fadeOutInput->setSampleId(-1);
        }
    }

    return false;
}