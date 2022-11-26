#ifndef DEF_SAMPLEMANAGER_HPP
#define DEF_SAMPLEMANAGER_HPP

/**
  *

  SampleManager will load, mix, process and play
  the audio samples loaded with the interface.

  */

#include <juce_gui_extra/juce_gui_extra.h>

#include <atomic>

#include "NotificationArea.h"
#include "Sample.h"

//==============================================================================
// ReferenceCountedBuffer is a pointer to an AudioBuffer that includes
// a reference counter. It's convenient way to clear the samples
// that remains unused by any Sample.
class ReferenceCountedBuffer : public juce::ReferenceCountedObject {
 public:
  typedef juce::ReferenceCountedObjectPtr<ReferenceCountedBuffer> Ptr;

  ReferenceCountedBuffer(const juce::String& nameToUse, int numChannels,
                         int numSamples)
      : name(nameToUse), buffer(numChannels, numSamples) { }

  ~ReferenceCountedBuffer() { }

  juce::AudioSampleBuffer* getAudioSampleBuffer() { return &buffer; }

  int position = 0;

 private:
  juce::String name;
  juce::AudioSampleBuffer buffer;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReferenceCountedBuffer)
};
//==============================================================================


//==============================================================================
class SampleManager : public juce::PositionableAudioSource,
                      private juce::Thread {
 public:
  SampleManager(NotificationArea&);
  ~SampleManager();

  // when called, add sample from file path with position
  int addSample(juce::String, int64_t);
  bool filePathsValid(const juce::StringArray&);

  // inherited from audio source
  void getNextAudioBlock(const juce::AudioSourceChannelInfo&) override;
  void prepareToPlay(int, double) override;
  void releaseResources() override;

  // inherited from positionable audio source
  void setNextReadPosition(int64) override;
  int64 getNextReadPosition(int64) override;
  int64 getTotalLength() override;
  bool isLooping() override;
  void setLooping(bool) override;

 private:

  // we need a reference to the notification object
  NotificationArea &notificationManager;

  // file formats manager
  juce::AudioFormatManager formatManager;

  // play cursom position in audio frames
  atomic_int64_t playCursor;
  // list of Sample objects that retains references to their underlying
  // ReferenceCountedBuffer or position ?

  // list of AudioSampleBuffer or ReferenceCountedBuffer that are playing a
  // sample ?
  // TODO: read again the code for this class
  juce::ReferenceCountedArray<ReferenceCountedBuffer> buffers;

  /** NOTES FROM JUCE FORUM ON MIXING Audio Sources:
  You can connect your AudioTransportSources to a MixerAudioSource 10, and call
  the mixers’s getNextAudioBlock() instead (which will then call the individual
  ones of the transportSources and mix the signal automatically).

  Please note, if you are planning to write an audio engine like that, you
  should use a different approach, because the timing of play/stop events on
  AudioTransportSources is very coarse, and musically not usable, because:

  - start/stop happens on the message thread. There can be any kind of delay,
  when your GUI is busy with other things
  - start/stop can only happen at full buffers,
  so you get discrete events with a deviation of up to 20ms (with typical 512
  samples @ 48kHz) from the actual trigger

  Usually you write an audio mixer yourself, that can take any sample position
  into account inside individual buffers, and that can update the play/stop and
  position parameters in the audio thread, to stay synchronised with the rest of
  the audio.
  **/

  // SO TODO: Write my own PositionableMixerAudioSource inherited mixer.

  // TODO: Use AudioProcessorGraph instead ?

  // list of IIRFilterAudioSource audio sources to apply on Audio Samples ?
  // TODO: look up how to glue sources together.
  //       Enventually create tracks as PositionableAudioSource or AudioSource

  // a MixerAudioSource class that will mix all currently playing tracks.
  // TODO: check if we need to swap audio sources live with addInputSource.. or
  //       if we can leave one per sample and just shift samples positions as
  //       needed

  // used to manage background thread allocations
  void checkForFileToImport();
  void checkForBuffersToFree();

  // mutex for buffer swapping
  juce::SpinLock mutex;

  // a buffer to copy paste data in the audio thread
  juce::AudioBuffer<float> audioThreadBuffer;

  // A list of raw sample outputs
  // TODO: make sure it's impossible to play two times
  //        a track (and create glitches)
  juce::Array<juce::PositionableAudioSource*> tracks;

  // An array of "Sample" object.
  // They inherits from a PositionableAudioSource and read
  // from a tracks entry.
  // A Sample player can run dsp in getNextAudioBlock so that
  // it adds filtering on top of tracks.
  juce::Array<SamplePlayer> samplePlayers;

  // TODO: data structure to prevent two tracks pulling same tracks.

  // mutex to swap the path
  juce::CriticalSection pathMutex, mixbusMutex;

  // path of the next file to import
  juce::String filePathToImport;
  // position to import file at
  atomic_int64_t filePositionToImport;

  // master bus main
  juce::dsp::Gain<float> masterGain;
  // master bus limiter
  juce::dsp::Limiter<float> masterLimiter;

  // a buffer value to hold Processing specs for dsp prepare functions
  juce::dsp::ProcessSpec currentAudioSpec;

  // Notes on exporting:
  /**
    When you want to write this data to a file, you should create a
    FileOutputStream 6 object and pass it your destination file in the
    constructor and then create an AudioFormatWriter 5 using the WavAudioFormat
    3 class createWriterFor() method with your output stream as the first
    argument. You can then write a set of audio samples to the output stream
    using the AudioFormatWriter::writeFromAudioSampleBuffer() method.
  */

  // inherited from thread, this is where we will malloc and free stuff
  void run() override;
};
//==============================================================================

#endif  // DEF_SAMPLEMANAGER_HPP