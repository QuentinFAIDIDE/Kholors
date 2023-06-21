#include "IconsLoader.h"
#include "Icons.h"

#include "../Config.h"

IconsLoader::IconsLoader()
{
    playIcon = juce::Drawable::createFromImageData(Icons::play_svg, Icons::play_svgSize);
    playIcon->replaceColour(juce::Colours::white, COLOR_TEXT_DARKER);

    pauseIcon = juce::Drawable::createFromImageData(Icons::pause_svg, Icons::pause_svgSize);
    pauseIcon->replaceColour(juce::Colours::white, COLOR_TEXT_DARKER);

    startIcon = juce::Drawable::createFromImageData(Icons::start_svg, Icons::start_svgSize);
    startIcon->replaceColour(juce::Colours::white, COLOR_TEXT_DARKER);

    loopIcon = juce::Drawable::createFromImageData(Icons::loop_svg, Icons::loop_svgSize);
    loopIcon->replaceColour(juce::Colours::white, COLOR_TEXT_DARKER);

    unloopIcon = juce::Drawable::createFromImageData(Icons::unloop_svg, Icons::unloop_svgSize);
    unloopIcon->replaceColour(juce::Colours::white, COLOR_TEXT_DARKER);

    moveIcon = juce::Drawable::createFromImageData(Icons::move_svg, Icons::move_svgSize);
    moveIcon->replaceColour(juce::Colours::white, COLOR_BACKGROUND);

    resizeHorizontalIcon =
        juce::Drawable::createFromImageData(Icons::resize_horizontal_svg, Icons::resize_horizontal_svgSize);
    resizeHorizontalIcon->replaceColour(juce::Colours::white, COLOR_BACKGROUND);
}