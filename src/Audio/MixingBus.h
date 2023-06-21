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
#include <memory>

#include "../Arrangement/ActivityManager.h"
#include "DataSource.h"
#include "MixbusDataSource.h"
#include "ReferenceCountedBuffer.h"
#include "SamplePlayer.h"

#define TASK_QUEUE_RESERVED_SIZE 16

//==============================================================================
class MixingBus : public juce::PositionableAudioSource, public TaskListener, private juce::Thread
{
  public:
    MixingBus(ActivityManager &);
    ~MixingBus();

    // this is the handler for the app's broadcasted tasks
    bool taskHandler(std::shared_ptr<Task> task) override;

    // when called, add sample from file path with position
    bool filePathsValid(const juce::StringArray &);

    // inherited from audio source
    void getNextAudioBlock(const juce::AudioSourceChannelInfo &) override;
    void prepareToPlay(int, double) override;
    void releaseResources() override;

    // inherited from positionable audio source
    void setNextReadPosition(juce::int64) override;
    juce::int64 getNextReadPosition() const override;
    juce::int64 getTotalLength() const override;
    bool isLooping() const override;
    void setLooping(bool) override;

    bool isCursorPlaying() const;

    // access tracks from gui's MessageThread
    size_t getNumTracks() const;
    std::shared_ptr<SamplePlayer> getTrack(int index) const;

    // set callback to safely access gui's
    // MessageThread to repaint tracks
    void setTrackRepaintCallback(std::function<void()>);

    /**
     * @brief      Gets a pointer to the mixbus data source.
     *
     * @return     The mixbus data source shared pointer.
     */
    std::shared_ptr<MixbusDataSource> getMixbusDataSource();

  private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixingBus)
    // TODO: add a readahead buffer
    // just like AudioTransportSource implementation

    ActivityManager &activityManager;

    // file formats manager
    juce::AudioFormatManager formatManager;
    // play cursom position in audio frames, as well as furthest frame
    int playCursor, totalFrameLength;

    // number of channels
    juce::int64 numChannels;

    // is the track currently playing ?
    bool isPlaying;

    // is the loop mode on ?
    bool loopingToggledOn;

    // position of the ends of the loop section in audio frames
    int64_t loopSectionStartFrame, loopSectionEndFrame;

    // a shared pointer to an instance of a MixbusDataSource we can share with GUI
    // (it has a builtin lock)
    std::shared_ptr<MixbusDataSource> mixbusDataSource;

    VuMeterData vuMeterVolumes;

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

    // a buffer to hold the summed selected samples signal
    juce::AudioBuffer<float> audioThreadSelectionBuffer;

    // A list of SamplePlayer objects that inherits PositionableAudioSource
    // and are objects that play buffers at some position
    juce::Array<std::shared_ptr<SamplePlayer>> tracks;
    // callback to repaint when tracks were updated
    std::function<void()> trackRepaintCallback;

    // helps deciding on notifying ArrangementArea for redraw
    int64_t lastDrawnCursor;

    // list of ReferenceCountedBuffer that are holding sample data
    juce::ReferenceCountedArray<ReferenceCountedBuffer> buffers;

    // mutex to swap the path and access tracks
    juce::CriticalSection pathMutex, mixbusMutex;

    // stack of files to import
    std::vector<std::shared_ptr<SampleCreateTask>> importTaskQueue;

    // master bus gain
    juce::dsp::Gain<float> masterGain;

    // a buffer value to hold Processing specs for dsp prepare functions
    juce::dsp::ProcessSpec currentAudioSpec;

    // used for fast fourier transforms of buffers
    juce::dsp::FFT forwardFFT;

    UserInterfaceState &uiState;

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
    void checkForCursorRedraw();

    void startPlayback();
    void stopPlayback();

    void importNewFile(std::shared_ptr<SampleCreateTask> task);
    void duplicateTrack(std::shared_ptr<SampleCreateTask> task);

    // used to manage background thread allocations
    void checkForBuffersToFree();

    void addSample(std::shared_ptr<SampleCreateTask> import);
    void deleteSample(std::shared_ptr<SampleDeletionTask> task);
    void restoreSample(std::shared_ptr<SampleRestoreTask> task);

    /**
    Crop a sample based on a SampleTimeCropTask task received.
    The task will specify if we're changing time at the beginning
    or end of sample and by how much.
    */
    void cropSample(std::shared_ptr<SampleTimeCropTask>);

    /**
    Crop a sample based on a SampleFreqCropTask task received.
    The task will specify if we're changing filter freq at the beginning
    or end of sample and by how much.
    */
    void cropSample(std::shared_ptr<SampleFreqCropTask>);
};
//==============================================================================

#endif // DEF_MIXING_BUS_HPP