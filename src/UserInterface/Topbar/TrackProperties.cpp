#include "TrackProperties.h"

#include "../Section.h"
#include <memory>

#include "../../Arrangement/NumericInputId.h"
#include "../Widgets/NumericInput.h"
#include "../Widgets/TimeInfo.h"

#define TRACKPROP_MAX_LABEL_WIDTH 60
#define TRACKPROP_TEMPO_WIDTH 70

TrackProperties::TrackProperties(ActivityManager &am)
{
    std::shared_ptr<TimeInfo> timeInfo = std::make_shared<TimeInfo>();
    trackTimeInfoLine =
        std::make_shared<LabeledLineContainer>("Position:", timeInfo, TRACKPROP_MAX_LABEL_WIDTH, TIMEINFO_WIDTH);
    addAndMakeVisible(*trackTimeInfoLine);

    // make the tempo widget and connect to to task handling (for updating values around)
    std::shared_ptr<NumericInput> tempoInput = std::make_shared<NumericInput>(true, MIN_TEMPO, MAX_TEMPO, 1);
    tempoInput->setUnit("bpm");
    am.registerTaskListener(tempoInput.get());
    tempoInput->setNumericInputId(NUM_INPUT_ID_TEMPO);
    tempoInput->setActivityManager(&am);
    trackTempoLine =
        std::make_shared<LabeledLineContainer>("Tempo:", tempoInput, TRACKPROP_MAX_LABEL_WIDTH, TRACKPROP_TEMPO_WIDTH);
    addAndMakeVisible(*trackTempoLine);
}

void TrackProperties::paint(juce::Graphics &g)
{
    auto bounds = g.getClipBounds();

    juce::Colour bg = juce::Colours::transparentBlack;
    drawSection(g, bounds, "Track Properties", bg, true);
}

void TrackProperties::resized()
{

    auto contentBounds = getLocalBounds();
    contentBounds.removeFromTop(SECTION_TITLE_HEIGHT_SMALL);

    trackTimeInfoLine->setBounds(contentBounds.removeFromTop(LABELED_LINE_CONTAINER_DEFAULT_HEIGHT));

    trackTempoLine->setBounds(contentBounds.removeFromTop(LABELED_LINE_CONTAINER_DEFAULT_HEIGHT));
}

void TrackProperties::setDataSource(std::shared_ptr<MixbusDataSource> ds)
{
    auto component = trackTimeInfoLine->getContent();
    std::shared_ptr<TimeInfo> timeInfo = std::dynamic_pointer_cast<TimeInfo>(component);
    if (timeInfo != nullptr)
    {
        std::shared_ptr<PositionDataSource> pds = std::dynamic_pointer_cast<PositionDataSource>(ds);
        if (pds != nullptr)
        {
            timeInfo->setDataSource(pds);
        }
    }
}