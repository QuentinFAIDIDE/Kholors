#include "SampleProperties.h"

#include "../Section.h"

SampleProperties::SampleProperties(ActivityManager &am)
{
    // TODO
}

void SampleProperties::paint(juce::Graphics &g)
{
    auto bounds = g.getClipBounds();

    juce::Colour bg = juce::Colours::transparentBlack;
    drawSection(g, bounds, "Selected Sample Properties", bg, true);
}

void SampleProperties::resized()
{
    // TODO
}