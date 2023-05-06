#ifndef DEF_AUDIO_LIBRARY_TAB
#define DEF_AUDIO_LIBRARY_TAB

#include <functional>

#include "../Arrangement/ActivityManager.h"
#include "../Config.h"
#include "../Library/AudioLibraryManager.h"
#include "AudioLibTreeItem.h"
#include "ColouredTreeView.h"
#include "LibrarySearchBar.h"
#include "ResultList.h"
#include "Section.h"

class AudioLibraryTab : public juce::Component,
                        public TaskListener,
                        public juce::TextEditor::Listener,
                        private juce::Thread
{
  public:
    AudioLibraryTab();
    ~AudioLibraryTab();

    /**
     * Instanciate the audio library manager with the given configuration.
     * @param config Config object that tells things like library locations.
     */
    void initAudioLibrary(Config &config);

    /**
     * Callback where task are broadcasted. We are listening
     * to Audio Library Count tasks that pops when user imports something.
     * @param  task the task going down the list of listeners.
     * @return      true if we are stopping it from going to further listeners, false if not.
     */
    bool taskHandler(std::shared_ptr<Task> task) override;

    /**
     * Called whenever the search bar text gets updated.
     * @param te The text editor widget object that has the text within it.
     */
    void textEditorTextChanged(juce::TextEditor &te) override;

    /**
     * Repaint the ui widgets.
     */
    void paint(juce::Graphics &) override;

    /**
     * called when the window/widget is resized.
     */
    void resized() override;

  private:
    AudioLibraryManager *audioLibraries;
    std::vector<std::string> audioLibPathsCopy;
    ColouredTreeView treeView;
    AudioLibTreeRoot *audioLibTreeRoot;

    juce::SpinLock searchTextLock;
    juce::String searchText;

    LibrarySearchBar searchBar;

    ResultList resultListContent;
    juce::ListBox resultList;

    juce::Rectangle<int> findLocation;
    juce::Rectangle<int> librariesSectionLocation;

    /**
     * This add an audio library to the audio library manager.
     * @param path path to audio library on disk
     */
    void addAudioLibrary(std::string path);

    /**
     * Get from the audio library maanger the list of top most
     * used entries and set the list content. This is
     * called when no text is in the search bar.
     */
    void updateBestEntries();

    /**
     * Set the content of the result list to nothing.
     */
    void emptyResultEntries();

    /**
     * Fetch from the background thread the search results from
     * audio libraries and set them in the result list widget from the
     * message thread (get message thread lock).
     * Make sure to call it from the background thread.
     * @param searchtxt text to search samples after.
     */
    void populateSearchContent(std::string searchtxt);

    /**
     * Sanitize path and tests if it's one of the library paths.
     * @param  path audio library disk path.
     * @return      true if it is, false if not.
     */
    bool isLibraryPath(std::string path);

    /**
     * Main loop of the background thread that gets called regularly or
     * when we call notify (when text is updated in search bar).
     * Responsible for conducting file searches for the string
     * in searchText untill it's empty (swapping it everytime).
     */
    void run() override;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioLibraryTab)
};

#endif // DEF_AUDIO_LIBRARY_TAB