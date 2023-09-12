#include "AudioLibTreeItem.h"

#include "../../Config.h"
#include <algorithm>

#define TREEVIEW_ICONS_AREA_WIDTH 30
#define TREEVIEW_ICONS_INNER_REDUCED 5

AudioLibTreeRoot::AudioLibTreeRoot()
{
}

AudioLibTreeRoot::~AudioLibTreeRoot()
{
    // NOTE: Juce already remove the tree items
}

bool AudioLibTreeRoot::mightContainSubItems()
{
    // should always contain at least one library folder
    return true;
}

bool AudioLibTreeRoot::canBeSelected() const
{
    return false;
}

void AudioLibTreeRoot::addAudioLibrary(std::string path)
{
    addSubItem((juce::TreeViewItem *)new AudioLibFile(path));
    std::cout << "addded new library to widget: " << path << std::endl;
}

bool AudioLibTreeRoot::customComponentUsesTreeViewMouseHandler() const
{
    return true;
}

void AudioLibTreeRoot::focusAtPath(std::string path)
{
    // destination file
    juce::File destFile(path);
    // iterate over childs
    for (int i = 0; i < getNumSubItems(); i++)
    {
        AudioLibFile *f = dynamic_cast<AudioLibFile *>(getSubItem(i));
        if (f != nullptr)
        {
            // is the file a child or the file we are looking for ?
            if (destFile.isAChildOf(f->getJuceFile()) ||
                destFile.getFullPathName() == f->getJuceFile().getFullPathName())
            {
                // if so, call the file's focusAtPath method
                f->focusAtPath(path);
                return;
            }
        }
    }
}

// ============ Audio Lib File =====================
AudioLibFile::AudioLibFile(std::string path) : file(path)
{
    isFolder = file.isDirectory();
    name = file.getFileName();
    hasLoadedChildren = false;
    // OPTIMISATION: load items from here if UI is too laggy at the price
    // of tons of disk I/O at startup
    setDrawsInLeftMargin(true);
}

int AudioLibFile::getItemHeight() const
{
    return TREEVIEW_ITEM_HEIGHT;
}

AudioLibFile::~AudioLibFile()
{
    // NOTE: Juce already remove the tree items
}

bool AudioLibFile::mightContainSubItems()
{
    return isFolder;
}

bool AudioLibFile::canBeSelected() const
{
    return true;
}

juce::String AudioLibFile::getUniqueName() const
{
    return name;
}

void AudioLibFile::itemOpennessChanged(bool isNowOpen)
{
    if (isNowOpen && !hasLoadedChildren)
    {
        juce::Array<juce::File> children = file.findChildFiles(juce::File::TypesOfFileToFind::findFilesAndDirectories,
                                                               false, "*", juce::File::FollowSymlinks::no);

        std::sort(children.begin(), children.end(), [](const juce::File &a, const juce::File &b) {
            if (a.isDirectory() != b.isDirectory())
            {
                return a.isDirectory();
            }
            return a.getFileName() < b.getFileName();
        });

        for (int i = 0; i < children.size(); i++)
        {
            if (children[i].isDirectory() || children[i].getFileExtension() == ".wav" ||
                children[i].getFileExtension() == ".mp3")
            {
                addSubItem((juce::TreeViewItem *)new AudioLibFile(children[i].getFullPathName().toStdString()));
            }
        }
        hasLoadedChildren = true;
    }
}

void AudioLibFile::paintItem(juce::Graphics &g, int width, int height)
{

    auto area = g.getClipBounds();

    area.setX(0);
    area.setY(0);
    area.setHeight(height);
    area.setWidth(width);

    auto iconArea = area.removeFromLeft(TREEVIEW_ICONS_AREA_WIDTH);
    iconArea.reduce(TREEVIEW_ICONS_INNER_REDUCED, TREEVIEW_ICONS_INNER_REDUCED);
    if (isFolder)
    {
        sharedIcons->folderIcon->drawWithin(g, iconArea.toFloat(), juce::RectanglePlacement::centred, 1.0f);
    }
    else
    {
        sharedIcons->audioIcon->drawWithin(g, iconArea.toFloat(), juce::RectanglePlacement::centred, 1.0f);
    }

    g.setColour(COLOR_TEXT_DARKER);
    g.setFont(juce::Font(DEFAULT_FONT_SIZE));
    g.drawText(name, area, juce::Justification::left);
    g.setColour(COLOR_TEXT_DARKER.withAlpha(0.4f));
}

void AudioLibFile::paintOpenCloseButton(juce::Graphics &g, const juce::Rectangle<float> &area,
                                        juce::Colour backgroundColour, bool isMouseOver)
{
    g.setColour(juce::Colour(180, 180, 180));
    auto middlePoint = area.reduced(area.getHeight() / 2).expanded(2);
    if (isOpen())
    {
        g.fillRect(middlePoint);
    }
    else
    {
        g.fillEllipse(middlePoint);
    }
}

juce::var AudioLibFile::getDragSourceDescription()
{
    return juce::var("file:" + std::string(file.getFullPathName().toStdString()));
}

bool AudioLibFile::customComponentUsesTreeViewMouseHandler() const
{
    return true;
}

juce::File &AudioLibFile::getJuceFile()
{
    return file;
}

void AudioLibFile::focusAtPath(std::string path)
{
    // destination file
    juce::File destFile(path);

    // if we are the file, stop and select ourselves
    if (destFile.getFullPathName() == file.getFullPathName())
    {
        setSelected(true, true);
        if (file.isDirectory())
        {
            setOpen(true);
        }
        getOwnerView()->scrollToKeepItemVisible((juce::TreeViewItem *)this);
        return;
    }

    // buggy situation where we are not the file and we're not
    // a directory.
    if (!file.isDirectory())
    {
        return;
    }

    // open ourselves as we're on the path
    setOpen(true);

    // iterate over childs
    for (int i = 0; i < getNumSubItems(); i++)
    {
        AudioLibFile *f = dynamic_cast<AudioLibFile *>(getSubItem(i));
        if (f != nullptr)
        {
            // is the file a child or the file we are looking for ?
            if (destFile.isAChildOf(f->getJuceFile()) ||
                destFile.getFullPathName() == f->getJuceFile().getFullPathName())
            {
                // if so, call the file's focusAtPath method
                f->focusAtPath(path);
                return;
            }
        }
    }
}