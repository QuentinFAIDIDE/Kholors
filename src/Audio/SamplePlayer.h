#ifndef DEF_SAMPLEPLAYER_HPP
#define DEF_SAMPLEPLAYER_HPP

// CMake builds don't use an AppConfig.h, so it's safe to include juce module
// headers directly. If you need to remain compatible with Projucer-generated
// builds, and have called `juce_generate_juce_header(<thisTarget>)` in your
// CMakeLists.txt, you could `#include <JuceHeader.h>` here instead, to make all
// your module headers visible.
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include <atomic>

#include "ReferenceCountedBuffer.h"
#include "UnitConverter.h"

#include "../Config.h"

class SamplePlayer : public juce::PositionableAudioSource
{
  public:
    SamplePlayer(int64_t position);
    ~SamplePlayer();
    // this tells the SamplePlayer which audio buffer to use
    void setBuffer(BufferPtr, juce::dsp::FFT &);
    void setBuffer(BufferPtr, std::vector<float> &fftData);

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
    std::vector<float> &getFftData();

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

  private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SamplePlayer)

    int fadeInFrameLength, fadeOutFrameLength;

    // the position from which bufferStart is mapped into the global
    // app play position
    int editingPosition;

    // a buffer value we keep off the stack for no reason
    int bufferInitialPosition;

    // bufferStart is the index from which starts the played section
    int bufferStart;
    // bufferEnd is the index above which the buffer is not read anymore.
    // Note that this is effectively the index of the last included audio frame.
    int bufferEnd;

    // this position is the one shared with all tracks (cursor pos)
    int position;

    // frequency of low pass filter. Disabled if equal to AUDIO_FRAMERATE
    float lowPassFreq;
    // frequency of high pass filter. Disabled if equal to 0
    float highPassFreq;

    BufferPtr audioBufferRef;
    bool isSampleSet;
    juce::Colour colour;

    // the sample gain (not in db but in Gain)
    float gainValue;

    // Store the results of the fft of the buffered audio.
    // LAYOUT: for each channel, for each fft over time, for each intensity at
    // freq. fft is size FREQVIEW_SAMPLE_FFT_SCOPE_SIZE and there are numFft. an
    // fft covers FREQVIEW_SAMPLE_FFT_SIZE audio samples.
    std::vector<float> audioBufferFrequencies;
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
    void checkRampsAreValid();
};

#endif // DEF_SAMPLEPLAYER_HPP