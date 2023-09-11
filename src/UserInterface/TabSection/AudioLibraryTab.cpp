#include "AudioLibraryTab.h"

#include <memory>
#include <stdexcept>

#include "../Section.h"

#define SEARCHBAR_HEIGHT 30
#define SEARCHBAR_BOTTOM_MARGIN 5

AudioLibraryTab::AudioLibraryTab() : Thread("File Search Thread"), resultList("Results", &resultListContent)
{
    startThread();

    audioLibTreeRoot = new AudioLibTreeRoot();
    treeView.setRootItem(audioLibTreeRoot);
    treeView.setRootItemVisible(false);
    treeView.setRepaintsOnMouseActivity(false);

    treeView.setColour(juce::TreeView::ColourIds::linesColourId, COLOR_TEXT.withAlpha(0.8f));
    treeView.setColour(juce::TreeView::ColourIds::selectedItemBackgroundColourId, COLOR_SELECTED_BACKGROUND);
    treeView.getViewport()->setScrollBarsShown(false, false, true, true);

    treeView.setIndentSize(TREEVIEW_INDENT_SIZE);

    resultList.setColour(juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    resultList.setColour(juce::ListBox::outlineColourId, juce::Colours::transparentBlack);
    resultList.getViewport()->setScrollBarsShown(false, false, true, true);
    resultList.setRowHeight(TREEVIEW_ITEM_HEIGHT);
    resultList.setOutlineThickness(0);

    searchBar.setColour(juce::TextEditor::ColourIds::textColourId, COLOR_TEXT);
    searchBar.setColour(juce::TextEditor::ColourIds::backgroundColourId, juce::Colours::transparentBlack);
    searchBar.setColour(juce::TextEditor::outlineColourId, COLOR_SEPARATOR_LINE);
    searchBar.setColour(juce::TextEditor::focusedOutlineColourId, COLOR_SEPARATOR_LINE);

    juce::Font font(DEFAULT_FONT_SIZE);
    searchBar.setFont(font);

    addAndMakeVisible(treeView);
    addAndMakeVisible(searchBar);
    addAndMakeVisible(resultList);

    // tell the result list where where to call for element focus
    resultListContent.focusElementCallback = [this](std::string path) { audioLibTreeRoot->focusAtPath(path); };

    searchBar.addListener(this);
}

bool AudioLibraryTab::taskHandler(std::shared_ptr<Task> task)
{
    std::shared_ptr<ImportFileCountTask> fctask = std::dynamic_pointer_cast<ImportFileCountTask>(task);

    if (fctask != nullptr)
    {

        // this can be called from the SampleManager worker thread, so
        // we need to lock the message/gui thread
        const juce::MessageManagerLock mmLock;

        juce::File f(fctask->path);
        while (!isLibraryPath(f.getFullPathName().toStdString()) && f != juce::File("/home") && f != juce::File("/"))
        {
            audioLibraries->countAccess(f);
            f = f.getParentDirectory();
        }
        // count the raw libraries (make sense for sorting em !)
        if (isLibraryPath(f.getFullPathName().toStdString()))
        {
            audioLibraries->countAccess(f);
        }
        updateBestEntries();

        return true;
    }

    return false;
}

bool AudioLibraryTab::isLibraryPath(std::string path)
{
    std::string pathWithoutLastSlash = path;
    if (path.find_last_of("/") == path.size() - 1)
    {
        pathWithoutLastSlash = path.substr(0, path.size() - 1);
    }

    for (int i = 0; i < audioLibPathsCopy.size(); i++)
    {
        if (pathWithoutLastSlash == audioLibPathsCopy[i])
        {
            return true;
        }
    }

    return false;
}

AudioLibraryTab::~AudioLibraryTab()
{
    // stop thread with a 4sec timeout to kill it
    stopThread(4000);

    treeView.setRootItem(nullptr);
    delete audioLibTreeRoot;
    if (audioLibraries != nullptr)
    {
        delete audioLibraries;
    }
}

void AudioLibraryTab::initAudioLibrary(Config &conf)
{
    audioLibraries = new AudioLibraryManager(conf.getDataFolderPath(), conf.getProfileName());
    for (int i = 0; i < conf.getNumAudioLibs(); i++)
    {
        addAudioLibrary(conf.getAudioLibPath(i));
    }
    updateBestEntries();
}

void AudioLibraryTab::addAudioLibrary(std::string path)
{
    if (audioLibraries != nullptr)
    {
        try
        {
            audioLibraries->addAudioLibrary(path);
            audioLibTreeRoot->addAudioLibrary(path);

            // do not keep trailing slash in the copy list.
            // note that this list is used to stop at library
            // path when counting uses of parent folder of used samples.
            std::string pathWithoutLastSlash = path;
            if (path.find_last_of("/") == path.size() - 1)
            {
                pathWithoutLastSlash = path.substr(0, path.size() - 1);
            }
            audioLibPathsCopy.push_back(pathWithoutLastSlash);
        }
        catch (std::runtime_error err)
        {
            std::cerr << "Unable to add library: " << err.what() << std::endl;
        }
    }
}

void AudioLibraryTab::paint(juce::Graphics &g)
{
    juce::Line<int> searchBarBottom(searchBarBounds.getBottomLeft(), searchBarBounds.getBottomRight());
    g.setColour(COLOR_TEXT_DARKER.withAlpha(0.5f));
    g.drawLine(searchBarBottom.toFloat(), 1.0f);
    // TODO: draw a lens instead
}

void AudioLibraryTab::resized()
{
    // get window coordinations
    juce::Rectangle<int> localBounds = getLocalBounds();
    localBounds.reduce(TAB_PADDING, TAB_PADDING);

    // set position of the Find section
    int idealProportion = LIBRARY_IDEAL_SEARCH_SIZE_PROPORTION * localBounds.getWidth();
    findLocation = localBounds.removeFromLeft(idealProportion).reduced(TAB_SECTIONS_MARGINS, TAB_SECTIONS_MARGINS);

    // set the position of the library files section
    treeView.setBounds(localBounds.reduced(TAB_SECTIONS_MARGINS, TAB_SECTIONS_MARGINS));

    // positionate the searchbar
    auto searchBarAndFileListArea = findLocation;
    searchBarBounds = searchBarAndFileListArea.removeFromTop(SEARCHBAR_HEIGHT);

    searchBar.setBounds(searchBarBounds);
    searchBarAndFileListArea.removeFromTop(SEARCHBAR_BOTTOM_MARGIN);
    resultList.setBounds(searchBarAndFileListArea);
}

void AudioLibraryTab::textEditorTextChanged(juce::TextEditor &te)
{
    juce::String txt = te.getText();

    // if text was just deleted, reset the result list
    if (txt.isEmpty())
    {
        updateBestEntries();
        {
            const juce::SpinLock::ScopedLockType lock(searchTextLock);
            searchText.clear();
        }
        return;
    }

    // if less than 3 character, do no search and display nothing
    if (txt.length() < 3)
    {
        emptyResultEntries();
        {
            const juce::SpinLock::ScopedLockType lock(searchTextLock);
            searchText.clear();
        }
        return;
    }

    // if >= 3 characters, swap the string and notifies the background thread
    {
        const juce::SpinLock::ScopedLockType lock(searchTextLock);
        searchText.swapWith(txt);
    }

    notify();
}

void AudioLibraryTab::updateBestEntries()
{
    std::vector<std::string> bestEntries = audioLibraries->getTopEntries(100);
    resultListContent.setContent(bestEntries);
    resultList.updateContent();
}

void AudioLibraryTab::emptyResultEntries()
{
    resultListContent.emptyContent();
    resultList.updateContent();
}

// Reminder: only call from this class background thread
void AudioLibraryTab::populateSearchContent(std::string txt)
{
    // let's only fetch a few results
    auto res = audioLibraries->getSearchResults(txt, 200);

    // lock the message thread and update the widget with the results
    const juce::MessageManagerLock mmLock;
    resultListContent.setContent(res);
    resultList.updateContent();
}

void AudioLibraryTab::run()
{
    while (!threadShouldExit())
    {

        juce::String stringToSearch = "";
        {
            const juce::SpinLock::ScopedLockType lock(searchTextLock);
            stringToSearch.swapWith(searchText);
        }

        if (stringToSearch.isNotEmpty())
        {
            populateSearchContent(stringToSearch.toStdString());
        }

        // wait 1000ms or untill notify() is called
        wait(1000);
    }
}