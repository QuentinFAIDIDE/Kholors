#include "AudioLibTreeItem.h"

AudioLibTreeRoot::AudioLibTreeRoot() {}

AudioLibTreeRoot::~AudioLibTreeRoot() {
  // NOTE: Juce already remove the tree items
}

bool AudioLibTreeRoot::mightContainSubItems() {
  // should always contain at least one library folder
  return true;
}

bool AudioLibTreeRoot::canBeSelected() const { return false; }

void AudioLibTreeRoot::addAudioLibrary(std::string path) {
  addSubItem((juce::TreeViewItem*)new AudioLibFile(path));
  std::cout << "addded new library to widget: " << path << std::endl;
}

// ============ Audio Lib File =====================
AudioLibFile::AudioLibFile(std::string path) : _file(path) {
  _isFolder = _file.isDirectory();
  _name = _file.getFileName();
  _hasLoadedChildren = false;
  // OPTIMISATION: load items from here if UI is too laggy at the price
  // of tons of disk I/O at startup
}

AudioLibFile::~AudioLibFile() {
  // NOTE: Juce already remove the tree items
}

bool AudioLibFile::mightContainSubItems() { return _isFolder; }

bool AudioLibFile::canBeSelected() const { return false; }

juce::String AudioLibFile::getUniqueName() const { return _name; }

void AudioLibFile::itemOpennessChanged(bool isNowOpen) {
  if (isNowOpen && !_hasLoadedChildren) {
    juce::Array<juce::File> children = _file.findChildFiles(
        juce::File::TypesOfFileToFind::findFilesAndDirectories, false, "*",
        juce::File::FollowSymlinks::no);

    for (int i = 0; i < children.size(); i++) {
      addSubItem((juce::TreeViewItem*)new AudioLibFile(
          children[i].getFullPathName().toStdString()));
    }
    _hasLoadedChildren = true;
  }
}

void AudioLibFile::paintItem(juce::Graphics& g, int width, int height) {
  g.setColour(juce::Colour(20, 20, 20));
  g.fillAll();
  g.setColour(juce::Colour(220, 220, 220));
  g.drawText(_name, 0, 0, width, height, juce::Justification::left);
}