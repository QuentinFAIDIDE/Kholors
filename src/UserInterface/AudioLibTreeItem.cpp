#include "AudioLibTreeItem.h"

#include <algorithm>

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
  addSubItem((juce::TreeViewItem *)new AudioLibFile(path));
  std::cout << "addded new library to widget: " << path << std::endl;
}

bool AudioLibTreeRoot::customComponentUsesTreeViewMouseHandler() const {
  return true;
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

bool AudioLibFile::canBeSelected() const { return !_isFolder; }

juce::String AudioLibFile::getUniqueName() const { return _name; }

void AudioLibFile::itemOpennessChanged(bool isNowOpen) {
  if (isNowOpen && !_hasLoadedChildren) {
    juce::Array<juce::File> children = _file.findChildFiles(
        juce::File::TypesOfFileToFind::findFilesAndDirectories, false, "*",
        juce::File::FollowSymlinks::no);

    std::sort(children.begin(), children.end(),
              [](const juce::File &a, const juce::File &b) {
                if (a.isDirectory() != b.isDirectory()) {
                  return a.isDirectory();
                }
                return a.getFileName() < b.getFileName();
              });

    for (int i = 0; i < children.size(); i++) {
      if (children[i].isDirectory() ||
          children[i].getFileExtension() == ".wav" ||
          children[i].getFileExtension() == ".mp3") {
        addSubItem((juce::TreeViewItem *)new AudioLibFile(
            children[i].getFullPathName().toStdString()));
      }
    }
    _hasLoadedChildren = true;
  }
}

void AudioLibFile::paintItem(juce::Graphics &g, int width, int height) {
  if (isSelected()) {
    g.setColour(juce::Colour::fromFloatRGBA(0.87, 0.8, 0.8, 0.12));
    g.fillAll();
  }
  g.setColour(juce::Colour(220, 220, 220));
  g.drawText(_name, 0, 0, width, height, juce::Justification::left);
}

void AudioLibFile::paintOpenCloseButton(juce::Graphics &g,
                                        const juce::Rectangle<float> &area,
                                        juce::Colour backgroundColour,
                                        bool isMouseOver) {
  g.setColour(juce::Colour(180, 180, 180));
  if (isOpen()) {
    g.fillRect(area.reduced(7, 8));
  } else {
    g.fillEllipse(area.reduced(7));
  }
}

juce::var AudioLibFile::getDragSourceDescription() {
  return juce::var("file:" +
                   std::string(_file.getFullPathName().toStdString()));
}

bool AudioLibFile::customComponentUsesTreeViewMouseHandler() const {
  return true;
}