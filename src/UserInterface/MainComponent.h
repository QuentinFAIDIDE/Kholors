#ifndef DEF_MAINCOMPONENT_HPP
#define DEF_MAINCOMPONENT_HPP

// CMake builds don't use an AppConfig.h, so it's safe to include juce module
// headers directly. If you need to remain compatible with Projucer-generated
// builds, and have called `juce_generate_juce_header(<thisTarget>)` in your
// CMakeLists.txt, you could `#include <JuceHeader.h>` here instead, to make all
// your module headers visible.
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include "ArrangementArea.h"
#include "../Audio/SampleManager.h"
#include "NotificationArea.h"

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent : public juce::AudioAppComponent, public juce::DragAndDropContainer {
 public:
  //==============================================================================
  MainComponent();
  ~MainComponent();

  //==============================================================================
  // inherited from Component from AudioAppComponent
  void paint(juce::Graphics&) override;
  void resized() override;

  // inherited from AudioSource from AudioAppComponent
  void prepareToPlay(int, double) override;
  void releaseResources() override;
  void getNextAudioBlock(const juce::AudioSourceChannelInfo&) override;

 private:
  //==============================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
  // widgets
  ArrangementArea arrangementArea;
  NotificationArea notificationArea;

  // the object responsible for managing the various samples imported
  SampleManager sampleManager;
};

#endif  // DEF_MAINCOMPONENT_HPP