#include "AudioLibraryTab.h"

#include <stdexcept>

AudioLibraryTab::AudioLibraryTab()
    : _searchSection("Search"),
      _topUsedSection("Most Used"),
      _locationsSection("Samples Locations") {
  addAndMakeVisible(_searchSection);
  addAndMakeVisible(_topUsedSection);
  // addAndMakeVisible(_locationsSection);

  _audioLibTreeRoot = new AudioLibTreeRoot();
  _treeView.setRootItem(_audioLibTreeRoot);
  _treeView.setRootItemVisible(false);
  addAndMakeVisible(_treeView);
}

AudioLibraryTab::~AudioLibraryTab() {
  delete _audioLibTreeRoot;
  if (_audioLibraries != nullptr) {
    delete _audioLibraries;
  }
}

void AudioLibraryTab::initAudioLibrary(Config& conf) {
  _audioLibraries =
      new AudioLibraryManager(conf.getDataFolderPath(), conf.getProfileName());
  for (int i = 0; i < conf.getNumAudioLibs(); i++) {
    _addAudioLibrary(conf.getAudioLibPath(i));
  }
}

void AudioLibraryTab::_addAudioLibrary(std::string path) {
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
  _treeView.setBounds(localBounds);
}