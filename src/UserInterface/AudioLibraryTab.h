#ifndef DEF_AUDIO_LIBRARY_TAB
#define DEF_AUDIO_LIBRARY_TAB

#include <functional>

#include "../Arrangement/ActivityManager.h"
#include "../Config.h"
#include "../Library/AudioLibraryManager.h"
#include "AudioLibTreeItem.h"
#include "ColouredTreeView.h"
#include "ResultList.h"
#include "Section.h"

class AudioLibraryTab : public juce::Component, public TaskListener
{
  public:
    AudioLibraryTab();
    ~AudioLibraryTab();

    void initAudioLibrary(Config &config);

    bool taskHandler(std::shared_ptr<Task> task) override;

    void paint(juce::Graphics &) override;
    void resized() override;

  private:
    AudioLibraryManager *audioLibraries;
    std::vector<std::string> audioLibPathsCopy;
    ColouredTreeView treeView;
    AudioLibTreeRoot *audioLibTreeRoot;

    juce::TextEditor searchBar;

    ResultList resultListContent;
    juce::ListBox resultList;

    juce::Rectangle<int> findLocation;
    juce::Rectangle<int> librariesSectionLocation;

    void addAudioLibrary(std::string path);
    void updateBestEntries();
    bool isLibraryPath(std::string path);

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioLibraryTab)
};

#endif // DEF_AUDIO_LIBRARY_TAB