#ifndef DEF_MIXING_BUS_HPP
#define DEF_MIXING_BUS_HPP

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include <atomic>
#include <functional>
#include <memory>

#include "../Arrangement/ActivityManager.h"
#include "AudioFilesBufferStore.h"
#include "DataSource.h"
#include "MixbusDataSource.h"
#include "SamplePlayer.h"

#define TASK_QUEUE_RESERVED_SIZE 16

/**
 * @brief MixingBus will load, mix, process and play
 *        the audio samples. It manages the main audio mixbus.
 *
 */
class MixingBus : public juce::PositionableAudioSource, public TaskListener, public Marshalable, private juce::Thread
{
  public:
    /**
     * @brief Construct a new Mixing Bus object
     *
     */
    MixingBus(ActivityManager &);

    /**
     * @brief Destroy the Mixing Bus object
     *
     */
    ~MixingBus();

    /**
     * @brief reset the state of the mixbus to a fresh new one. (used for reseting app state.)
     */
    void reset();

    /**
     * @brief This is where we receive tasks and decide if we act on them.
     *
     * @param task The task that is broadcasted to all listeners.
     * @return true If we stop and intercept the broadcasting of the task now (will still go to history).
     * @return false If we let the task continue its way down the listeners.
     */
    bool taskHandler(std::shared_ptr<Task> task) override;

    //
    /**
     * @brief Initially supposed to check that path for imported files are valid, but we decided to fail later
     *         and this function is both deprecated and blocked to returning always true.
     *
     * @return true is always returned
     * @return false is never returned
     */
    bool filePathsValid(const juce::StringArray &);

    /**
     * @brief Get the Next Audio Block object. What it specifically does here is called
     *        getAudioBlock once, or twice if reaching the loop end.
     *
     * @param asci Audio buffer to fill with necessary informations.
     */
    void getNextAudioBlock(const juce::AudioSourceChannelInfo &) override;

    /**
     * @brief Get a block of audio from the sample players. Used in getNextAudioBlock
     *        to trick it into handling end of loop (by separating the AudioSourceChannelInfo in two parts).
     *        DOES NOT TAKE THE MIXBUS LOCK, TAKE IT BEFORE CALLING IT !
     *
     * @param asci Audio buffer to fill with necessary informations.
     */
    void getAudioBlock(const juce::AudioSourceChannelInfo &asci);

    /**
     * @brief Juce inherited method to prepare resources to start the audio thread playbook.
     *
     */
    void prepareToPlay(int, double) override;

    /**
     * @brief Release audio resources at audio thread stop.
     *
     */
    void releaseResources() override;

    /**
     * @brief Set the next read position in all sample players. Note that it does not takes
     *        the mixbus lock, and you should take it before calling it !
     */
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

    /**
     * @brief      Dump the state of the Mixbus into a JSON string.
     *
     * @return     a JSON format string describing the state of the mixbus.
     */
    std::string marshal() override;

    /**
     * @brief      Parse the JSON string and restore the mixbus state.
     *
     * @param      s     A string representing the mixbus.
     */
    void unmarshal(std::string &s) override;

  private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixingBus)
    // TODO: add a readahead buffer
    // just like AudioTransportSource implementation

    ActivityManager &activityManager;

    int playCursor; /**< Play cursor position in audio frames. This is the index of the next audio frame to be read. */

    // number of channels
    juce::int64 numChannels;

    // is the track currently playing ?
    bool isPlaying;

    // is the loop mode on ?
    bool loopingToggledOn;

    // Position of the ends of the loop section in audio frames. Loop should
    // include start frame and include the end frame.
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

    juce::SharedResourcePointer<AudioFilesBufferStore>
        sharedAudioFileBuffers; /**< object managing audio buffers read from files */

    // A list of SamplePlayer objects that inherits PositionableAudioSource
    // and are objects that play buffers at some position
    juce::Array<std::shared_ptr<SamplePlayer>> samplePlayers;
    // callback to repaint when tracks were updated
    std::function<void()> trackRepaintCallback;

    // helps deciding on notifying ArrangementArea for redraw
    int64_t lastDrawnCursor;

    // mutex to swap the path and access tracks
    juce::CriticalSection pathMutex, mixbusMutex;

    // master bus gain
    juce::dsp::Gain<float> masterGain;

    // a buffer value to hold Processing specs for dsp prepare functions
    juce::dsp::ProcessSpec currentAudioSpec;

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