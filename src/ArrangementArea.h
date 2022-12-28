#ifndef DEF_ARRANGEMENTAREA_HPP
#define DEF_ARRANGEMENTAREA_HPP

// CMake builds don't use an AppConfig.h, so it's safe to include juce module
// headers directly. If you need to remain compatible with Projucer-generated
// builds, and have called `juce_generate_juce_header(<thisTarget>)` in your
// CMakeLists.txt, you could `#include <JuceHeader.h>` here instead, to make all
// your module headers visible.
#include <juce_gui_extra/juce_gui_extra.h>

#include "Config.h"
#include "GridLevel.h"
#include "NotificationArea.h"
#include "SampleManager.h"

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class ArrangementArea : public juce::Component, public juce::FileDragAndDropTarget {
 public:
  //==============================================================================
  ArrangementArea(SampleManager& sm, NotificationArea& na);
  ~ArrangementArea();

  //==============================================================================
  void paint(juce::Graphics&) override;
  void resized() override;
  void mouseDown(const juce::MouseEvent&) override;
  void mouseUp(const juce::MouseEvent&) override;
  void mouseDrag(const juce::MouseEvent&) override;
  void mouseMove(const juce::MouseEvent&) override;
  bool isInterestedInFileDrag(const juce::StringArray&) override;
  void filesDropped(const juce::StringArray&, int, int) override;

 private:
  //==============================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangementArea)

  // the index in audio frame of the view (relation to seconds depends on
  // framerate)
  int64_t viewPosition;
  // how many audio frames per pixel to display
  int64_t viewScale;
  // tempo in beats per minute
  int tempo;
  // size and position of main content widget
  juce::Rectangle<int> bounds;
  // last mouse coordinates
  int lastMouseX;
  int lastMouseY;
  // levels to display bars
  std::vector<GridLevel> gridSubdivisions;
  // are we in resize mode ? (middle mouse button pressed)
  bool isResizing;
  // reference to the sample manager in use
  SampleManager& sampleManager;
  NotificationArea& notificationArea;

  //==============================================================================
  void paintBars(juce::Graphics&);
  void paintSamples(juce::Graphics&);
  void drawSampleTrack(juce::Graphics&, SamplePlayer*, int64_t);
};

#endif  // DEF_ARRANGEMENTAREA_HPP