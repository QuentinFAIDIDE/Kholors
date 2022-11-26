#ifndef DEF_SAMPLEMANAGER_HPP
#define DEF_SAMPLEMANAGER_HPP

/**
  *

  SampleManager will load, mix, process and play
  the audio samples loaded with the interface.

  */

#include <juce_gui_extra/juce_gui_extra.h>

#include "Sample.h"

class SampleManager: public juce::AudioAppComponent,
                     private juce::Thread {
 public:
  SampleManager();
  ~SampleManager();

  // when called, add sample from file path with position
  int addSample(juce::String, int64_t);
  bool filePathsValid(const juce::StringArray&);
  
  // inherited from audio source
  void getNextAudioBlock (const juce::AudioSourceChannelInfo&) override;
  void prepareToPlay(int, double) override;
  void releaseResources() override;

private:

  // file formats manager
  juce::AudioFormatManager formatManager;

  // play cursom position in audio frames
  int64_t playCursor;
  // list of Sample objects that retains references to their underlying ReferenceCountedBuffer or position ?
  
  // list of AudioSampleBuffer or ReferenceCountedBuffer that are playing a sample ?
  // TODO: read again the code for this class
  juce::ReferenceCountedArray<ReferenceCountedBuffer> buffers;

  // list of IIRFilterAudioSource audio sources to apply on Audio Samples ?
  // TODO: look up how to glue sources together.
  //       Enventually create tracks as PositionableAudioSource or AudioSource
  
  // a MixerAudioSource class that will mix all currently playing tracks.
  // TODO: check if we need to swap audio sources live with addInputSource.. or
  //       if we can leave one per sample and just shift samples positions as needed

  // used to manage background thread allocations
  void checkForFileToImport();
  void checkForBuffersToFree();

  // mutex for buffer swapping
  juce::SpinLock mutex;
  
  ReferenceCountedBuffer::Ptr currentBuffer;
  
  // mutex to swap the path
  // TODO: read on CriticalSection locks and SpinLock
  juce::CriticalSection pathMutex;

  // path of the next file to import
  juce::String filePathToImport;
  // position to import file at
  int64_t filePositionToImport;

  // inherited from thread
  void run() override;
};

#endif  // DEF_SAMPLEMANAGER_HPP