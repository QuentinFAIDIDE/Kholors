#ifndef DEF_AUDIOLIB_TREE_ROOT
#define DEF_AUDIOLIB_TREE_ROOT

#include <juce_gui_extra/juce_gui_extra.h>

class AudioLibTreeRoot : public juce::TreeViewItem {
 public:
  ~AudioLibTreeRoot();
  bool mightContainSubItems() override;
  bool canBeSelected() const override;
  void addAudioLibrary(std::string);
};

class AudioLibFile : public juce::TreeViewItem {
 public:
  AudioLibFile(std::string path);
  ~AudioLibFile();
  bool mightContainSubItems() override;
  bool canBeSelected() const override;
  juce::String getUniqueName() const override;
  void itemOpennessChanged(bool) override;

 private:
  juce::File _file;
  bool _isFolder;
  juce::String _name;
  bool _hasLoadedChildren;
};

#endif  // DEF_AUDIOLIB_TREE_ROOT