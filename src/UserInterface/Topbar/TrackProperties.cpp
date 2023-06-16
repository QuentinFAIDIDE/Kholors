#include "TrackProperties.h"

#include "../Section.h"
#include <memory>

#include "../Widgets/NumericInput.h"
#include "../Widgets/TimeInfo.h"

#define TRACKPROP_MAX_LABEL_WIDTH 60
#define TRACKPROP_TEMPO_WIDTH 50

TrackProperties::TrackProperties()
{
    std::shared_ptr<TimeInfo> timeInfo = std::make_shared<TimeInfo>();
    trackTimeInfoLine =
        std::make_shared<LabeledLineContainer>("Position:", timeInfo, TRACKPROP_MAX_LABEL_WIDTH, TIMEINFO_WIDTH);
    addAndMakeVisible(*trackTimeInfoLine);

    std::shared_ptr<NumericInput> tempoInput = std::make_shared<NumericInput>(true, 40, 200, 1, true);
    tempoInput->setValue(130);
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