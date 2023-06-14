#include "TrackProperties.h"

#include "../Section.h"
#include <memory>

#include "../Widgets/NumericInput.h"
#include "../Widgets/TimeInfo.h"

TrackProperties::TrackProperties()
{
    std::shared_ptr<TimeInfo> timeInfo = std::make_shared<TimeInfo>();
    trackTimeInfoLine = std::make_shared<LabeledLineContainer>("Position:", timeInfo, 60, TIMEINFO_WIDTH);
    addAndMakeVisible(*trackTimeInfoLine);

    std::shared_ptr<NumericInput> tempoInput = std::make_shared<NumericInput>(true, 40, 200, 1, true);
    tempoInput->setValue(130);
    trackTempoLine = std::make_shared<LabeledLineContainer>("Tempo:", tempoInput, 60, 70);
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