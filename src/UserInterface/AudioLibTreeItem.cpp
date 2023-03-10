#include "AudioLibTreeItem.h"

AudioLibTreeRoot::~AudioLibTreeRoot() {
  // remove chlidren nodes
  for (int i = 0; i < getNumSubItems(); i++) {
    delete getSubItem(i);
  }
}

bool AudioLibTreeRoot::mightContainSubItems() {
  // should always contain at least one library folder
  return true;
}

bool AudioLibTreeRoot::canBeSelected() const { return false; }

void AudioLibTreeRoot::addAudioLibrary(std::string path) {
  addSubItem((juce::TreeViewItem*)new AudioLibFile(path));
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
  // remove chlidren nodes
  for (int i = 0; i < getNumSubItems(); i++) {
    delete getSubItem(i);
  }
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