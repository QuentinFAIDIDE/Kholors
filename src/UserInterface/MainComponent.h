#ifndef DEF_MAINCOMPONENT_HPP
#define DEF_MAINCOMPONENT_HPP

// CMake builds don't use an AppConfig.h, so it's safe to include juce module
// headers directly. If you need to remain compatible with Projucer-generated
// builds, and have called `juce_generate_juce_header(<thisTarget>)` in your
// CMakeLists.txt, you could `#include <JuceHeader.h>` here instead, to make all
// your module headers visible.
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "../Arrangement/UserInterfaceState.h"
#include "../Audio/MixingBus.h"
#include "./ArrangementArea/ArrangementArea.h"
#include "KholorsLookAndFeel.h"
#include "MenuBar.h"
#include "TabSection/AudioLibraryTab.h"
#include "TabSection/EmptyTab.h"
#include "TabSection/GeneratorsTab.h"
#include "Topbar/TopbarArea.h"

#include "MenuBar.h"
#include "Widgets/PassClickTab.h"

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent : public juce::AudioAppComponent, public juce::DragAndDropContainer, public TaskListener
{
  public:
    //==============================================================================
    MainComponent();
    ~MainComponent();

    //==============================================================================
    // inherited from Component from AudioAppComponent
    void paint(juce::Graphics &) override;
    void resized() override;

    // inherited from AudioSource from AudioAppComponent
    void prepareToPlay(int, double) override;
    void releaseResources() override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo &) override;
    void configureApp(Config &conf);

    bool taskHandler(std::shared_ptr<Task> task) override;

    void mouseMove(const juce::MouseEvent &me) override;
    void mouseUp(const juce::MouseEvent &me) override;
    void mouseDown(const juce::MouseEvent &me) override;
    void mouseDrag(const juce::MouseEvent &me) override;

  private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)

    ActivityManager activityManager;

    // widgets
    // the object responsible for managing the various samples imported
    MixingBus mixingBus;

    ArrangementArea arrangementArea;
    int arrangementAreaHeight;

    juce::Rectangle<int> resizeHandleArea;

    TopbarArea topbarArea;
    ResizableTabsContainer actionTabs;

    AudioLibraryTab audioLibraryTab;
    GeneratorsTab generatorsTab;
    EmptyTab sampleProcessingTab;
    EmptyTab masteringTab;

    MenuBar menu;

    juce::SharedResourcePointer<Config> sharedConfig;

    KholorsLookAndFeel appLookAndFeel;
    void configureLookAndFeel();

    void printAudioDeviceSettings();
};

#endif // DEF_MAINCOMPONENT_HPP