#ifndef DEF_MIXING_BUS_HPP
#define DEF_MIXING_BUS_HPP

/**
  *

  MixingBus will load, mix, process and play
  the audio samples loaded with the interface.

  */

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include <atomic>
#include <functional>

#include "../UserInterface/NotificationArea.h"
#include "ReferenceCountedBuffer.h"
#include "SamplePlayer.h"

//==============================================================================
class MixingBus : public juce::PositionableAudioSource, private juce::Thread {
 public:
  MixingBus(NotificationArea&);
  ~MixingBus();

  // when called, add sample from file path with position
  void addSample(juce::String, int64_t);
  bool filePathsValid(const juce::StringArray&);

  // inherited from audio source
  void getNextAudioBlock(const juce::AudioSourceChannelInfo&) override;
  void prepareToPlay(int, double) override;
  void releaseResources() override;

  // inherited from positionable audio source
  void setNextReadPosition(juce::int64) override;
  juce::int64 getNextReadPosition() const override;
  juce::int64 getTotalLength() const override;
  bool isLooping() const override;
  void setLooping(bool) override;

  // start and stop playing
  void startPlayback();
  void stopPlayback();
  bool isCursorPlaying() const;

  // access tracks from gui's MessageThread
  size_t getNumTracks() const;
  SamplePlayer* getTrack(int index) const;
  SamplePlayer* deleteTrack(int index);
  void restoreDeletedTrack(SamplePlayer* sp, int index);

  int duplicateTrack(int index, int newPos);

  // set callback to safely access gui's
  // MessageThread to repaint tracks
  void setTrackRepaintCallback(std::function<void()>);
  void setFileImportedCallback(std::function<void(std::string)>);

  // callback to add new samples to the user interface
  std::function<void(SamplePlayer*)> addUiSampleCallback;
  // callback to add new samples to the user interface
  std::function<void(int)> disableUiSampleCallback;

 private:
  // TODO: add a readahead buffer
  // just like AudioTransportSource implementation

  // we need a reference to the notification object
  NotificationArea& notificationManager;
  // file formats manager
  juce::AudioFormatManager formatManager;
  // play cursom position in audio frames, as well as furthest frame
  int playCursor, totalFrameLength;

  // number of channels
  juce::int64 numChannels;

  // is the track currently playing ?
  bool isPlaying;

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

  // mutex for buffer swapping
  juce::SpinLock mutex;

  // a buffer to copy paste data in the audio thread
  juce::AudioBuffer<float> audioThreadBuffer;

  // A list of SamplePlayer objects that inherits PositionableAudioSource
  // and are objects that play buffers at some position
  juce::Array<SamplePlayer*> tracks;
  // callback to repaint when tracks were updated
  std::function<void()> trackRepaintCallback;
  // callback to report the file was imported to audio library
  std::function<void(std::string)> fileImportedCallback;

  // helps deciding on notifying ArrangementArea for redraw
  int64_t lastDrawnCursor;
  void checkForCursorRedraw();

  // list of ReferenceCountedBuffer that are holding sample data
  juce::ReferenceCountedArray<ReferenceCountedBuffer> buffers;

  // here is bitmask to identify which tracks are nearby the play cursor
  int64_t* nearTracksBitmask;
  // the one we fill before swapping pointers
  int64_t* backgroundNearTrackBitmask;

  // mutex to swap the path and access tracks
  juce::CriticalSection pathMutex, mixbusMutex;

  // path of the next file to import
  juce::String filePathToImport;
  // position to import file at
  std::atomic_int64_t filePositionToImport;

  // master bus main
  juce::dsp::Gain<float> masterGain;
  // master bus limiter
  juce::dsp::Limiter<float> masterLimiter;

  // a buffer value to hold Processing specs for dsp prepare functions
  juce::dsp::ProcessSpec currentAudioSpec;

  // used for fast fourier transforms of buffers
  juce::dsp::FFT forwardFFT;

  // Notes on exporting:
  /**
    When you want to write this data to a file, you should create a
    FileOutputStream 6 object and pass it your destination file in the
    constructor and then create an AudioFormatWriter 5 using the WavAudioFormat
    3 class createWriterFor() method with your output stream as the first
    argument. You can then write a set of audio samples to the output stream
    using the AudioFormatWriter::writeFromAudioSampleBuffer() method.
  */

  // ===========================================================================

  // inherited from thread, this is where we will malloc and free stuff
  void run() override;

  // this stop the cursor from moving forward an cuts audio when nothing is
  // playing anymore
  void pauseIfCursorNotInBound();

  // used to manage background thread allocations
  void checkForFileToImport();
  void checkForBuffersToFree();
};
//==============================================================================

#endif  // DEF_MIXING_BUS_HPP