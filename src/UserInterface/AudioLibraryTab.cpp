#include "AudioLibraryTab.h"

#include <stdexcept>

#include "Section.h"

AudioLibraryTab::AudioLibraryTab() {
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
  juce::Colour bgColor(30, 29, 29);

  drawSection(g, _mostUsedSectionLocation, "Most Used", bgColor);
  drawSection(g, _librariesSectionLocation, "Libraries", bgColor);
}

void AudioLibraryTab::resized() {
  // get window coordinations
  juce::Rectangle<int> localBounds = getLocalBounds();
  localBounds.reduce(6, 6);

  int widthHalf = localBounds.getWidth() >> 1;
  localBounds.setWidth(widthHalf);

  _mostUsedSectionLocation = localBounds.reduced(5, 5);

  localBounds.setX(localBounds.getX() + widthHalf);
  _librariesSectionLocation = localBounds.reduced(5, 5);

  localBounds.reduce(5, 5);
  localBounds.setY(localBounds.getY() + 22);
  localBounds.setHeight(localBounds.getHeight() - 25);
  _treeView.setBounds(localBounds.reduced(2));
}