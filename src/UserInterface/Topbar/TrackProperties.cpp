#include "TrackProperties.h"

#include "../Section.h"
#include <memory>

#include "../Widgets/TimeInfo.h"

TrackProperties::TrackProperties()
{
    std::shared_ptr<TimeInfo> timeInfo = std::make_shared<TimeInfo>();
    trackTimeInfoLine = std::make_shared<LabeledLineContainer>("Position:", timeInfo, 60, TIMEINFO_WIDTH);
    addAndMakeVisible(*trackTimeInfoLine);
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
}