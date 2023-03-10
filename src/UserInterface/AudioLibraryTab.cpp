#include "AudioLibraryTab.h"

#include <stdexcept>

AudioLibraryTab::AudioLibraryTab()
    : _searchSection("Search"),
      _topUsedSection("Most Used"),
      _locationsSection("Samples Locations") {
  _audioLibTreeRoot = new AudioLibTreeRoot;
  _treeView.setRootItem(_audioLibTreeRoot);

  _locationsSection.setContent(&_treeView);

  addAndMakeVisible(_searchSection);
  addAndMakeVisible(_topUsedSection);
  addAndMakeVisible(_locationsSection);

  _locationsSection.setContent(&_treeView);
}

AudioLibraryTab::~AudioLibraryTab() { delete _audioLibTreeRoot; }

void AudioLibraryTab::addAudioLibrary(std::string path) {
  if (_audioLibraries != nullptr) {
    try {
      _audioLibraries->addAudioLibrary(path);
      _audioLibTreeRoot->addAudioLibrary(path);
    } catch (std::runtime_error err) {
      std::cerr << "Unable to add library: " << err.what() << std::endl;
    }
  }
}

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