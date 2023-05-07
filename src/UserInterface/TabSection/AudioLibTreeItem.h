#ifndef DEF_AUDIOLIB_TREE_ROOT
#define DEF_AUDIOLIB_TREE_ROOT

#include <juce_gui_extra/juce_gui_extra.h>

class AudioLibTreeRoot : public juce::TreeViewItem
{
  public:
    AudioLibTreeRoot();
    ~AudioLibTreeRoot();
    bool mightContainSubItems() override;
    bool canBeSelected() const override;
    void addAudioLibrary(std::string);
    bool customComponentUsesTreeViewMouseHandler() const override;
    void focusAtPath(std::string);

  private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioLibTreeRoot)
};

class AudioLibFile : public juce::TreeViewItem
{
  public:
    AudioLibFile(std::string path);
    ~AudioLibFile();
    bool mightContainSubItems() override;
    bool canBeSelected() const override;
    juce::String getUniqueName() const override;
    void itemOpennessChanged(bool) override;
    void paintItem(juce::Graphics &g, int width, int height) override;
    void paintOpenCloseButton(juce::Graphics &, const juce::Rectangle<float> &area, juce::Colour backgroundColour,
                              bool isMouseOver) override;
    juce::var getDragSourceDescription() override;
    bool customComponentUsesTreeViewMouseHandler() const override;
    juce::File &getJuceFile();
    void focusAtPath(std::string);

  private:
    juce::File file;
    bool isFolder;
    juce::String name;
    bool hasLoadedChildren;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioLibFile)
};

#endif // DEF_AUDIOLIB_TREE_ROOT