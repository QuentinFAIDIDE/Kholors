#ifndef DEF_REFERENCE_COUNTED_BUFFER_HPP
#define DEF_REFERENCE_COUNTED_BUFFER_HPP

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include <atomic>

//==============================================================================
// ReferenceCountedBuffer is a pointer to an AudioBuffer that includes
// a reference counter. It's convenient way to clear the samples
// that remains unused by any Sample.
// Taken from juce tutorial: LoopingAudioSampleBufferAdvancedTutorial.
class ReferenceCountedBuffer : public juce::ReferenceCountedObject {
 public:
  ReferenceCountedBuffer(const juce::String& nameToUse, int numChannels,
                         int numSamples)
      : name(nameToUse), buffer(numChannels, numSamples) {}

  ~ReferenceCountedBuffer() {}

  juce::AudioSampleBuffer* getAudioSampleBuffer() { return &buffer; }

  std::string getName() { return name.toStdString(); };

 private:
  juce::String name;
  juce::AudioSampleBuffer buffer;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReferenceCountedBuffer)
};
typedef juce::ReferenceCountedObjectPtr<ReferenceCountedBuffer> BufferPtr;
//==============================================================================

#endif  // DEF_REFERENCE_COUNTED_BUFFER_HPP