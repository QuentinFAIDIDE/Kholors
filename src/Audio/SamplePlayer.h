#ifndef DEF_SAMPLEPLAYER_HPP
#define DEF_SAMPLEPLAYER_HPP

#include <atomic>

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <nlohmann/json.hpp>

#include "../Config.h"
#include "AudioFilesBufferStore.h"
#include "FftRunner.h"
#include "UnitConverter.h"

using json = nlohmann::json;

/**
 * @brief Describe a class that plays a sample and can be positioned
 *        in the track. These objects are owned by the mixbus.
 *
 */
class SamplePlayer : public juce::PositionableAudioSource
{
  public:
    SamplePlayer(int64_t position);
    ~SamplePlayer();

    // this tells the SamplePlayer which audio buffer to use
    void setBuffer(AudioFileBufferRef audioBufferRef);

    /**
     * @brief      Gets the buffer reference.
     *
     * @return     The buffer reference.
     */
    AudioFileBufferRef getBufferRef();

    // inherited from PositionableAudioSource
    juce::int64 getNextReadPosition() const override;
    void setNextReadPosition(juce::int64) override;
    juce::int64 getTotalLength() const override;
    bool isLooping() const override;

    // AudioSource inherited functions
    void prepareToPlay(int, double) override;
    void releaseResources() override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo &) override;

    // move the sample to a new track position
    void move(juce::int64);
    // set the length up to which read the buffer
    void setLength(juce::int64);
    // get the length up to which the buffer is readead
    juce::int64 getLength() const;

    // how many channels does the buffer has ?
    int getBufferNumChannels() const;

    // create and move a duplicate (uses same underlying audio buffer)
    std::shared_ptr<SamplePlayer> createDuplicate(juce::int64);

    /**
     * will split the sample in two at a frequency provided (returns new other half)
     */
    std::shared_ptr<SamplePlayer> splitAtFrequency(float frequencyLimitHz);

    /**
     * will split the sample in two at a time provided (returns new other half).
     * Time is relative to bufferStart (the position we start reading the buffer
     * at).
     * A positionLimit of 10 means that from bufferStart (0th) to bufferStart+9 (9th)
     * the current sample will play. Then the new play from bufferStart+10 (10th)
     * to bufferEnd included (= bufferStart + (getLength()-1)).
     */
    std::shared_ptr<SamplePlayer> splitAtPosition(juce::int64 positionLimit);

    int tryMovingStart(int desiredShift);
    int tryMovingEnd(int desiredShift);

    int getBufferStart() const;
    int getBufferEnd() const;

    void setLowPassFreq(int freq);
    void setHighPassFreq(int freq);
    float getLowPassFreq();
    float getHighPassFreq();

    void setGainRamp(float);

    std::string getFileName();

    // get number of fft blocks we use to cover the buffer
    int getNumFft() const;
    std::shared_ptr<std::vector<float>> getFftData();

    // a lock to switch buffers and safely read in message thread (gui)
    juce::SpinLock playerMutex;
    // helpers to read graphical properties
    int64_t getEditingPosition() const;
    bool hasBeenInitialized() const;

    static int maxFilterFreq;

    // set the shift for the buffer reading start position
    void setBufferShift(juce::int64);
    // get the shift of the buffer shift
    juce::int64 getBufferShift() const;

    /**
     * @brief      Gets the fade in length in audio frames.
     *
     * @return     The fade in length.
     */
    int getFadeInLength();

    /**
     * @brief      Sets the fade in length in audio frames.
     *
     * @param[in]  length  The length
     *
     * @return     true if it worked, false if not
     */
    bool setFadeInLength(int length);

    /**
     * @brief      Gets the fade out length in audio frames.
     *
     * @return     The fade out length.
     */
    int getFadeOutLength();

    /**
     * @brief      Sets the fade out length in audio frames.
     *
     * @param[in]  length  The length
     *
     * @return     true if it worked, false if not
     */
    bool setFadeOutLength(int length);

    /**
     * @brief       Sets the gain in decibels.
     */
    void setDbGain(float db);

    /**
     * @brief       Get the gain in decibels.
     */
    float getDbGain();

    /**
     * Return how many times the 12db/octave lowpass is repeated
     */
    int getLowPassRepeat();

    /**
     * Sets how many time the low pass filter repeats. Must be
     * between 1 and SAMPLEPLAYER_MAX_FILTER_REPEAT
     */
    void setLowPassRepeat(int repeat);

    /**
     * Return how many times the 12db/octave highpass is repeated
     */
    int getHighPassRepeat();

    /**
     * Sets how many time the high pass filter repeats. Must be
     * between 1 and SAMPLEPLAYER_MAX_FILTER_REPEAT
     */
    void setHighPassRepeat(int repeat);

    /**
     * @brief       Find the frequency at which the filter
     *              (low pass or high pass) will have reduced
     *              the amplitude by the specified amount.
     *
     * @param[in]  isHighPass           True if the filter is high pass, false if low pass
     * @param[in]  filterFreq           Frequency where the filter is applied (if we ignore knee, where the slope
     * starts)
     * @param[in]  dbReductionRequired  The db reduction we wish to have the frequency for.
     * @param[in]  filterRepeat         How many times the base 12db/octave filter is repeated
     *
     */
    static float freqForFilterDbReduction(bool isHighPass, float filterFreq, float dbReductionRequired,
                                          int filterRepeat);

    /**
     * @brief      Will drop all of this track properties to a JSON object.
     *
     * @return     Json representation of the object.
     */
    json toJSON();

    /**
     * @brief      Will restore the sample player state as its defined in this object.
     *
     * @param      object  The object containing the state to be restored.
     */
    void setupFromJSON(json &object);

  private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SamplePlayer)

    int fadeInFrameLength, fadeOutFrameLength;

    // the position from which bufferStart is mapped into the global
    // app play position
    int editingPosition;

    // a buffer value we keep off the stack for no reason
    int bufferInitialPosition;

    // bufferStart is the index from which starts the played section in audio frames
    // inside the audio buffer.
    int bufferStart;

    // bufferEnd is the index in audio frames of the audio buffer above which the buffer is not read anymore.
    // Note that this is effectively the index of the last included audio frame.
    int bufferEnd;

    // this position is the one shared with all tracks (cursor pos)
    int position;

    // frequency of low pass filter. Disabled if equal to AUDIO_FRAMERATE
    float lowPassFreq;

    // frequency of high pass filter. Disabled if equal to 0
    float highPassFreq;

    // how many 12db/octave filters we successively apply for low pass filtering ?
    int lowPassRepeat;

    // how many 12db/octave filters we successively apply for high pass filtering ?
    int highPassRepeat;

    AudioFileBufferRef audioBufferRef;
    bool isSampleSet;

    // the sample gain (not in db but in Gain)
    float gainValue;

    // Store the results of the fft of the buffered audio.
    // LAYOUT: for each channel, for each fft over time, for each intensity at
    // freq. fft is size FFT_STORAGE_SCOPE_SIZE and there are numFft. an
    // fft covers FREQVIEW_SAMPLE_FFT_SIZE audio samples.
    std::shared_ptr<std::vector<float>> audioBufferFrequencies;
    // how many blocks of FREQVIEW_SAMPLE_FFT_SIZE samples
    // for this buffer
    int numFft;

    juce::IIRFilter lowPassFilterLeft[SAMPLEPLAYER_MAX_FILTER_REPEAT];
    juce::IIRFilter lowPassFilterRight[SAMPLEPLAYER_MAX_FILTER_REPEAT];

    juce::IIRFilter highPassFilterLeft[SAMPLEPLAYER_MAX_FILTER_REPEAT];
    juce::IIRFilter highPassFilterRight[SAMPLEPLAYER_MAX_FILTER_REPEAT];

    void applyFilters(const juce::AudioSourceChannelInfo &bufferToFill);
    void applyGainFade(float *data, int startIndex, int length, int startIndexLocalPositon);

    /**
     * addOnScreenAmountToFreq will add an amount to the frequency freq
     * equivalent to a movement of (screenProportion*100) % of the screen.
     */
    float addOnScreenAmountToFreq(float freq, float screenProportion);

    /**
     Checks if the gain fade in and fade out ramp sums to more than the length
     and if so, reduce them appropriately
     */
    void checkGainRamps();
};

#endif // DEF_SAMPLEPLAYER_HPP