#include "AudioLibraryTab.h"

AudioLibraryTab::AudioLibraryTab()
    : _searchSection("Search"),
      _topUsedSection("Most Used"),
      _locationsSection("Samples Locations") {
  addAndMakeVisible(_searchSection);
  addAndMakeVisible(_topUsedSection);
  addAndMakeVisible(_locationsSection);
}

AudioLibraryTab::~AudioLibraryTab() {}

void AudioLibraryTab::addAudioLibrary(std::string path) {}

void AudioLibraryTab::paint(juce::Graphics& g) {
  g.setColour(juce::Colour(20, 20, 20));
  g.fillAll();
}

void AudioLibraryTab::resized() {
  // get window coordinations
  juce::Rectangle<int> localBounds = getLocalBounds();
  localBounds.reduce(6, 6);

  int widthThird = localBounds.getWidth() / 3;
  localBounds.setWidth(widthThird);

  _searchSection.setBounds(localBounds);

  localBounds.setX(localBounds.getX() + widthThird);
  _topUsedSection.setBounds(localBounds);

  localBounds.setX(localBounds.getX() + widthThird);
  _locationsSection.setBounds(localBounds);
}