#include "SampleProperties.h"

#include "../Section.h"
#include "SampleInputs.h"
#include <climits>
#include <memory>

#define SAMPLEPROPS_MAX_LABEL_WIDTH 140
#define SAMPLEPROPS_INPUT_WIDTH 100

SampleProperties::SampleProperties(ActivityManager &am)
{
    auto fadeInInput = std::make_shared<SampleFadeInput>(true);
    fadeInInput->setUnit("ms");
    am.registerTaskListener(fadeInInput.get());
    fadeInInput->setActivityManager(&am);
    fadeInInput->setMinDragUpdate(1.0f);
    fadeInLine = std::make_shared<LabeledLineContainer>("Attack", fadeInInput, SAMPLEPROPS_MAX_LABEL_WIDTH,
                                                        SAMPLEPROPS_INPUT_WIDTH);
    addAndMakeVisible(*fadeInLine);

    auto fadeOutInput = std::make_shared<SampleFadeInput>(false);
    fadeOutInput->setUnit("ms");
    am.registerTaskListener(fadeOutInput.get());
    fadeOutInput->setActivityManager(&am);
    fadeOutInput->setMinDragUpdate(1.0f);
    fadeOutLine = std::make_shared<LabeledLineContainer>("Release", fadeOutInput, SAMPLEPROPS_MAX_LABEL_WIDTH,
                                                         SAMPLEPROPS_INPUT_WIDTH);
    addAndMakeVisible(*fadeOutLine);

    auto gainInput = std::make_shared<SampleGainInput>();
    gainInput->setUnit("dB");
    am.registerTaskListener(gainInput.get());
    gainInput->setActivityManager(&am);
    gainLine =
        std::make_shared<LabeledLineContainer>("Gain", gainInput, SAMPLEPROPS_MAX_LABEL_WIDTH, SAMPLEPROPS_INPUT_WIDTH);
    addAndMakeVisible(*gainLine);

    auto lpRepeatInput = std::make_shared<SampleFilterRepeatInput>(true);
    lpRepeatInput->setUnit("dB/oct");
    am.registerTaskListener(lpRepeatInput.get());
    lpRepeatInput->setActivityManager(&am);
    lpRepeatLine = std::make_shared<LabeledLineContainer>("LP Slope", lpRepeatInput, SAMPLEPROPS_MAX_LABEL_WIDTH,
                                                          SAMPLEPROPS_INPUT_WIDTH);
    addAndMakeVisible(*lpRepeatLine);

    auto hpRepeatInput = std::make_shared<SampleFilterRepeatInput>(false);
    hpRepeatInput->setUnit("dB/oct");
    am.registerTaskListener(hpRepeatInput.get());
    hpRepeatInput->setActivityManager(&am);
    hpRepeatLine = std::make_shared<LabeledLineContainer>("HP Slope", hpRepeatInput, SAMPLEPROPS_MAX_LABEL_WIDTH,
                                                          SAMPLEPROPS_INPUT_WIDTH);
    addAndMakeVisible(*hpRepeatLine);

    am.registerTaskListener(this);
}

void SampleProperties::paint(juce::Graphics &g)
{
    auto bounds = g.getClipBounds();
    drawSection(g, bounds, "Selected Sample Properties");
}

void SampleProperties::resized()
{
    auto contentBounds = getLocalBounds();
    contentBounds.removeFromTop(SECTION_TITLE_HEIGHT);
    gainLine->setBounds(contentBounds.removeFromTop(LABELED_LINE_CONTAINER_DEFAULT_HEIGHT));
    contentBounds.removeFromTop(LABELED_LINE_CONTAINER_SPACING);
    fadeInLine->setBounds(contentBounds.removeFromTop(LABELED_LINE_CONTAINER_DEFAULT_HEIGHT));
    contentBounds.removeFromTop(LABELED_LINE_CONTAINER_SPACING);
    fadeOutLine->setBounds(contentBounds.removeFromTop(LABELED_LINE_CONTAINER_DEFAULT_HEIGHT));
    contentBounds.removeFromTop(LABELED_LINE_CONTAINER_SPACING);
    lpRepeatLine->setBounds(contentBounds.removeFromTop(LABELED_LINE_CONTAINER_DEFAULT_HEIGHT));
    contentBounds.removeFromTop(LABELED_LINE_CONTAINER_SPACING);
    hpRepeatLine->setBounds(contentBounds.removeFromTop(LABELED_LINE_CONTAINER_DEFAULT_HEIGHT));
}

bool SampleProperties::taskHandler(std::shared_ptr<Task> task)
{
    auto selectionUpdateTask = std::dynamic_pointer_cast<SelectionChangingTask>(task);
    if (selectionUpdateTask != nullptr && selectionUpdateTask->isCompleted())
    {
        auto fadeInInput = std::dynamic_pointer_cast<SampleFadeInput>(fadeInLine->getContent());
        auto fadeOutInput = std::dynamic_pointer_cast<SampleFadeInput>(fadeOutLine->getContent());
        auto gainInput = std::dynamic_pointer_cast<SampleGainInput>(gainLine->getContent());
        auto lpRepeatInput = std::dynamic_pointer_cast<SampleFilterRepeatInput>(lpRepeatLine->getContent());
        auto hpRepeatInput = std::dynamic_pointer_cast<SampleFilterRepeatInput>(hpRepeatLine->getContent());

        if (fadeInInput == nullptr || fadeOutInput == nullptr || gainInput == nullptr || lpRepeatLine == nullptr ||
            hpRepeatLine == nullptr)
        {
            return false;
        }

        fadeInInput->setSampleIds(selectionUpdateTask->newSelectedTracks);
        fadeOutInput->setSampleIds(selectionUpdateTask->newSelectedTracks);
        gainInput->setSampleIds(selectionUpdateTask->newSelectedTracks);
        lpRepeatInput->setSampleIds(selectionUpdateTask->newSelectedTracks);
        hpRepeatInput->setSampleIds(selectionUpdateTask->newSelectedTracks);
    }

    return false;
}

int SampleProperties::getIdealHeight()
{
    return SECTION_TITLE_HEIGHT + (5 * (TABS_HEIGHT + LABELED_LINE_CONTAINER_SPACING));
}