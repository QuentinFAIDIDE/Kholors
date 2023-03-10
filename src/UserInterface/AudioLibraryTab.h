#ifndef DEF_AUDIO_LIBRARY_TAB
#define DEF_AUDIO_LIBRARY_TAB

#include "../Library/AudioLibraryManager.h"
#include "AudioLibTreeItem.h"
#include "Section.h"

class AudioLibraryTab : public juce::Component {
 public:
  AudioLibraryTab();
  ~AudioLibraryTab();

  void addAudioLibrary(std::string path);

  void paint(juce::Graphics &) override;
  void resized() override;

 private:
  Section _searchSection;
  Section _topUsedSection;
  Section _locationsSection;
  AudioLibraryManager *_audioLibraries;
  juce::TreeView _treeView;
  AudioLibTreeRoot *_audioLibTreeRoot;
};

#endif  // DEF_AUDIO_LIBRARY_TAB