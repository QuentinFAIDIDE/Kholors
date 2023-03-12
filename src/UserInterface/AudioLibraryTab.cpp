#include "AudioLibraryTab.h"

#include <stdexcept>

#include "Section.h"

AudioLibraryTab::AudioLibraryTab()
    : _resultList("Results", &_resultListContent) {
  // addAndMakeVisible(_locationsSection);

  _searchBar.setCaretVisible(true);
  _searchBar.setScrollbarsShown(false);
  _searchBar.setJustification(juce::Justification::left);
  _searchBar.setDescription("Search for files here");
  _searchBar.setMultiLine(false);
  _searchBar.setColour(juce::TextEditor::backgroundColourId,
                       juce::Colour(COLOR_APP_BACKGROUND));
  _searchBar.setColour(juce::TextEditor::outlineColourId,
                       juce::Colour::fromRGBA(255, 255, 255, 100));
  _searchBar.setColour(juce::TextEditor::focusedOutlineColourId,
                       juce::Colour::fromRGBA(255, 255, 255, 125));
  _searchBar.setColour(juce::TextEditor::textColourId, COLOR_NOTIF_TEXT);
  _searchBar.setMouseCursor(juce::MouseCursor::IBeamCursor);

  _audioLibTreeRoot = new AudioLibTreeRoot();
  _treeView.setRootItem(_audioLibTreeRoot);
  _treeView.setRootItemVisible(false);
  _treeView.setRepaintsOnMouseActivity(false);

  _resultList.setColour(juce::ListBox::backgroundColourId,
                        juce::Colour::fromFloatRGBA(0, 0, 0, 0));

  addAndMakeVisible(_treeView);
  addAndMakeVisible(_searchBar);
  addAndMakeVisible(_resultList);

  // callback to pass to sampleManager to report file import
  fileWasImported = [this](std::string s) {
    juce::File f(s);
    while (!_isLibraryPath(f.getFullPathName().toStdString()) &&
           f != juce::File("/home") && f != juce::File("/")) {
      _audioLibraries->countAccess(f);
      f = f.getParentDirectory();
    }
    // count the raw libraries (make sense for sorting em !)
    if (_isLibraryPath(f.getFullPathName().toStdString())) {
      _audioLibraries->countAccess(f);
    }
    _updateBestEntries();
  };
}

bool AudioLibraryTab::_isLibraryPath(std::string path) {
  std::string pathWithoutLastSlash = path;
  if (path.find_last_of("/") == path.size() - 1) {
    pathWithoutLastSlash = path.substr(0, path.size() - 1);
  }

  for (int i = 0; i < _audioLibPathsCopy.size(); i++) {
    if (pathWithoutLastSlash == _audioLibPathsCopy[i]) {
      return true;
    }
  }

  return false;
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
  _updateBestEntries();
}

void AudioLibraryTab::_updateBestEntries() {
  std::vector<std::string> bestEntries = _audioLibraries->getTopEntries(100);
  std::cerr << "Got " << bestEntries.size() << " top entries" << std::endl;
  _resultListContent.setContent(bestEntries);
  _resultList.updateContent();
}

void AudioLibraryTab::_addAudioLibrary(std::string path) {
  if (_audioLibraries != nullptr) {
    try {
      _audioLibraries->addAudioLibrary(path);
      _audioLibTreeRoot->addAudioLibrary(path);

      // do not keep trailing slash in the copy list.
      // note that this list is used to stop at library
      // path when counting uses of parent folder of used samples.
      std::string pathWithoutLastSlash = path;
      if (path.find_last_of("/") == path.size() - 1) {
        pathWithoutLastSlash = path.substr(0, path.size() - 1);
      }
      _audioLibPathsCopy.push_back(pathWithoutLastSlash);

    } catch (std::runtime_error err) {
      std::cerr << "Unable to add library: " << err.what() << std::endl;
    }
  }
}

void AudioLibraryTab::paint(juce::Graphics& g) {
  juce::Colour bgColor(30, 29, 29);

  drawSection(g, _findLocation, "Find", bgColor);
  drawSection(g, _librariesSectionLocation, "Libraries", bgColor);
}

void AudioLibraryTab::resized() {
  // get window coordinations
  juce::Rectangle<int> localBounds = getLocalBounds();
  localBounds.reduce(6, 6);

  // set position of the Find section
  int widthQuarter = localBounds.getWidth() >> 2;
  localBounds.setWidth(widthQuarter);
  _findLocation = localBounds.reduced(5, 5);

  // set the position of the library files section
  localBounds.setX(localBounds.getX() + widthQuarter);
  localBounds.setWidth(localBounds.getWidth() + (2 * widthQuarter));
  _librariesSectionLocation = localBounds.reduced(5, 5);

  // set the position of the file browser
  localBounds.reduce(5, 5);
  localBounds.setY(localBounds.getY() + 22);
  localBounds.setHeight(localBounds.getHeight() - 25);
  _treeView.setBounds(localBounds.reduced(2));

  // positionate the searchbar
  _searchBar.setBounds(_findLocation.reduced(5, 5)
                           .withHeight(26)
                           .withY(_findLocation.getY() + 24)
                           .reduced(2));
  _resultList.setBounds(
      _findLocation.reduced(5, 5)
          .withHeight(_findLocation.reduced(5, 5).getHeight() - 26 - 24 + 1)
          .withY(_findLocation.getY() + 24 + 26 + 3)
          .reduced(2));
}
