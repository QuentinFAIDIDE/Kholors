#ifndef DEF_SAMPLEMANAGER_HPP
#define DEF_SAMPLEMANAGER_HPP

// CMake builds don't use an AppConfig.h, so it's safe to include juce module
// headers directly. If you need to remain compatible with Projucer-generated
// builds, and have called `juce_generate_juce_header(<thisTarget>)` in your
// CMakeLists.txt, you could `#include <JuceHeader.h>` here instead, to make all
// your module headers visible.
#include <juce_gui_extra/juce_gui_extra.h>

#include "FileError.h"

class SampleManager {
 public:
  SampleManager();
  ~SampleManager();

  // when called, add sample from file path with coordinates
  int addSample(juce::String, int64_t);
  bool filePathsValid(const juce::StringArray&);
};

#endif  // DEF_SAMPLEMANAGER_HPP