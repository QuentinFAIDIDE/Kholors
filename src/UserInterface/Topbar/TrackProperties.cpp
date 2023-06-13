#include "TrackProperties.h"

#include "../Section.h"

TrackProperties::TrackProperties()
{
}

void TrackProperties::paint(juce::Graphics &g)
{
    auto bounds = g.getClipBounds();

    juce::Colour bg = juce::Colours::transparentBlack;
    drawSection(g, bounds, "Track Properties", bg, true);
}