#include "AudioLibraryTab.h"

#include <memory>
#include <stdexcept>

#include "../Section.h"

#define SEARCHBAR_HEIGHT 30
#define LENS_ICON_WIDTH 24 // prefer something even
#define LENS_ICON_AREA_WIDTH SEARCHBAR_HEIGHT
#define SEARCHBAR_BOTTOM_MARGIN 5
#define LENS_ADDITIONAL_LEFT_PADDING 3
#define TEXT_ENTRY_DISPLACEMENT 1 // used to correct text entry that is misplaced a few pixels vertically

AudioLibraryTab::AudioLibraryTab() : Thread("File Search Thread"), resultList("Results", &resultListContent)
{
    startThread();

    audioLibTreeRoot = new AudioLibTreeRoot();
    treeView.setRootItem(audioLibTreeRoot);
    treeView.setRootItemVisible(false);
    treeView.setRepaintsOnMouseActivity(false);

    treeView.getViewport()->setScrollBarsShown(true, false, true, true);

    treeView.setIndentSize(TREEVIEW_INDENT_SIZE);

    resultList.getViewport()->setScrollBarsShown(true, false, true, true);
    resultList.setRowHeight(TREEVIEW_ITEM_HEIGHT);
    resultList.setOutlineThickness(0);

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

void AudioLibraryTab::paintOverChildren(juce::Graphics &g)
{
    auto searchbarArea = searchBar.getBoundsInParent();
    searchbarArea.setY(searchbarArea.getY() + TEXT_ENTRY_DISPLACEMENT);
    // add the area where we plan to draw the lens icon
    searchbarArea.setX(searchbarArea.getX() - LENS_ICON_AREA_WIDTH);
    searchbarArea.setWidth(searchbarArea.getWidth() + LENS_ICON_AREA_WIDTH);
    g.setColour(COLOR_SEPARATOR_LINE);
    g.drawRect(searchbarArea);

    // draw a prototype of ugly lens
    auto lensArea = searchbarArea;
    lensArea = lensArea.removeFromLeft(LENS_ICON_AREA_WIDTH);
    int padding = (LENS_ICON_AREA_WIDTH - LENS_ICON_WIDTH);
    lensArea.reduce(padding, padding);
    lensArea.setX(lensArea.getX() + LENS_ADDITIONAL_LEFT_PADDING);
    sharedIcons->searchIcon->drawWithin(g, lensArea.toFloat(), juce::RectanglePlacement::centred, 1.0f);

    // if empty and we don't have the focus, draw a placeholder text
    if (searchBar.isEmpty() && !searchBar.hasKeyboardFocus(true))
    {
        auto placeholderArea = searchBarBounds.withY(searchBarBounds.getY() + TEXT_ENTRY_DISPLACEMENT);
        placeholderArea.reduce(4, 4);
        g.drawText("Search your library here...", placeholderArea, juce::Justification::centredLeft, true);
    }
}

void AudioLibraryTab::focusOfChildComponentChanged(FocusChangeType)
{
    // this allow us to repaint our placeholds when user
    // exit text area keyboard focus
    repaint();
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
    // remove the area where we want to draw the lens icon
    searchBarBounds.removeFromLeft(LENS_ICON_AREA_WIDTH);
    searchBarBounds.setY(searchBarBounds.getY() - TEXT_ENTRY_DISPLACEMENT);

    searchBar.setBounds(searchBarBounds);
    searchBarAndFileListArea.removeFromTop(SEARCHBAR_BOTTOM_MARGIN);
    resultList.setBounds(searchBarAndFileListArea);
}

void AudioLibraryTab::textEditorTextChanged(juce::TextEditor &te)
{
    // this can make sure our drawing on top of the text area update asap
    repaint();

    juce::String txt = te.getText();

    // if text was just deleted, reset the result list
    if (txt.isEmpty())
    {
        updateBestEntries();
        searchText.clear();
        return;
    }

    // if less than 3 character, do no search and display nothing
    if (txt.length() < 3)
    {
        emptyResultEntries();
        searchText.clear();
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

    // if the text is now different, do nothing
    if (searchBar.getText().toStdString() == txt)
    {
        resultListContent.setContent(res);
        resultList.updateContent();
    }
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