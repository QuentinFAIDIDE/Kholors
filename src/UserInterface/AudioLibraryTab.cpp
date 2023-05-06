#include "AudioLibraryTab.h"

#include <memory>
#include <stdexcept>

#include "Section.h"

AudioLibraryTab::AudioLibraryTab() : resultList("Results", &resultListContent)
{
    searchBar.setCaretVisible(true);
    searchBar.setScrollbarsShown(false);
    searchBar.setJustification(juce::Justification::left);
    searchBar.setDescription("Search for files here");
    searchBar.setMultiLine(false);
    searchBar.setColour(juce::TextEditor::backgroundColourId, juce::Colour(COLOR_APP_BACKGROUND));
    searchBar.setColour(juce::TextEditor::outlineColourId, juce::Colour::fromRGBA(255, 255, 255, 100));
    searchBar.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour::fromRGBA(255, 255, 255, 125));
    searchBar.setColour(juce::TextEditor::textColourId, COLOR_NOTIF_TEXT);
    searchBar.setMouseCursor(juce::MouseCursor::IBeamCursor);

    audioLibTreeRoot = new AudioLibTreeRoot();
    treeView.setRootItem(audioLibTreeRoot);
    treeView.setRootItemVisible(false);
    treeView.setRepaintsOnMouseActivity(false);

    resultList.setColour(juce::ListBox::backgroundColourId, juce::Colour::fromFloatRGBA(0, 0, 0, 0));

    addAndMakeVisible(treeView);
    addAndMakeVisible(searchBar);
    addAndMakeVisible(resultList);

    // tell the result list where where to call for element focus
    resultListContent.focusElementCallback = [this](std::string path) { audioLibTreeRoot->focusAtPath(path); };
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

void AudioLibraryTab::updateBestEntries()
{
    std::vector<std::string> bestEntries = audioLibraries->getTopEntries(100);
    resultListContent.setContent(bestEntries);
    resultList.updateContent();
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
    juce::Colour bgColor(30, 29, 29);

    drawSection(g, findLocation, "Find", bgColor);
    drawSection(g, librariesSectionLocation, "Libraries", bgColor);
}

void AudioLibraryTab::resized()
{
    // get window coordinations
    juce::Rectangle<int> localBounds = getLocalBounds();
    localBounds.reduce(6, 6);

    // set position of the Find section
    int idealProportion = LIBRARY_IDEAL_SEARCH_SIZE_PROPORTION * localBounds.getWidth();
    localBounds.setWidth(juce::jmax(idealProportion, LIBRARY_MIN_SEARCH_SIZE));
    findLocation = localBounds.reduced(5, 5);

    // set the position of the library files section
    localBounds.setX(localBounds.getX() + localBounds.getWidth());
    localBounds.setWidth(getLocalBounds().reduced(6, 6).getWidth() - localBounds.getWidth());
    librariesSectionLocation = localBounds.reduced(5, 5);

    // set the position of the file browser
    localBounds.reduce(5, 5);
    localBounds.setY(localBounds.getY() + 22);
    localBounds.setHeight(localBounds.getHeight() - 25);
    treeView.setBounds(localBounds.reduced(2));

    // positionate the searchbar
    searchBar.setBounds(findLocation.reduced(5, 5).withHeight(26).withY(findLocation.getY() + 24).reduced(2));
    resultList.setBounds(findLocation.reduced(5, 5)
                             .withHeight(findLocation.reduced(5, 5).getHeight() - 26 - 24 + 1)
                             .withY(findLocation.getY() + 24 + 26 + 3)
                             .reduced(2));
}
