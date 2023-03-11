#ifndef DEF_AUDIO_LIBRARY_TAB
#define DEF_AUDIO_LIBRARY_TAB

#include "../Config.h"
#include "../Library/AudioLibraryManager.h"
#include "AudioLibTreeItem.h"
#include "ColouredTreeView.h"
#include "Section.h"

class AudioLibraryTab : public juce::Component {
 public:
  AudioLibraryTab();
  ~AudioLibraryTab();

  void initAudioLibrary(Config &config);

  void paint(juce::Graphics &) override;
  void resized() override;

 private:
  AudioLibraryManager *_audioLibraries;
  ColouredTreeView _treeView;
  AudioLibTreeRoot *_audioLibTreeRoot;

  juce::Rectangle<int> _searchSectionLocation;
  juce::Rectangle<int> _mostUsedSectionLocation;
  juce::Rectangle<int> _librariesSectionLocation;

  void _addAudioLibrary(std::string path);

  //==============================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioLibraryTab)
};

#endif  // DEF_AUDIO_LIBRARY_TAB