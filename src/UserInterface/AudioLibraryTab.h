#ifndef DEF_AUDIO_LIBRARY_TAB
#define DEF_AUDIO_LIBRARY_TAB

#include "Section.h"

class AudioLibraryTab : public juce::Component {
 public:
  AudioLibraryTab();
  ~AudioLibraryTab();

  void addAudioLibrary(std::string path);

  void paint(juce::Graphics&) override;
  void resized() override;

 private:
  Section _searchSection;
  Section _topUsedSection;
  Section _locationsSection;
};

#endif  // DEF_AUDIO_LIBRARY_TAB