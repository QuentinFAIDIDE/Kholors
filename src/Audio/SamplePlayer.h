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

#include "../Config.h"

class SamplePlayer : public juce::PositionableAudioSource
{
  public:
    SamplePlayer(int64_t position);
    ~SamplePlayer();
    // this tells the SamplePlayer which audio buffer to use
    void setBuffer(BufferPtr, juce::dsp::FFT &);

    void setTrackIndex(int);
    int getTrackIndex();

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
    // set the shift for the buffer reading start position
    void setBufferShift(juce::int64);
    // get the shift of the buffer shift
    juce::int64 getBufferShift() const;
    // how many channels does the buffer has ?
    int getBufferNumChannels() const;
    // create and move a duplicate (uses same underlying audio buffer)
    SamplePlayer *createDuplicate(juce::int64, juce::dsp::FFT &);
    // will split the sample in two at a frquency provided
    // (returns new other half)
    SamplePlayer *split(float frequencyLimitHz);
    // will split the sample in two at the time provided
    // (returns new other half)
    SamplePlayer *split(juce::int64 positionLimit);

    int tryMovingStart(int desiredShift);
    int tryMovingEnd(int desiredShift);

    int getBufferStart() const;
    int getBufferEnd() const;

    void setLowPassFreq(int freq);
    void setHighPassFreq(int freq);

    std::string getFileName();

    // get number of fft blocks we use to cover the buffer
    int getNumFft() const;
    std::vector<float> &getFftData();

    // a lock to switch buffers and safely read in message thread (gui)
    juce::SpinLock playerMutex;
    // helpers to read graphical properties
    int64_t getEditingPosition() const;
    bool hasBeenInitialized() const;

    // TODO: getters for stereo low and high pass

  private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SamplePlayer)

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

    // Store the results of the fft of the buffered audio.
    // LAYOUT: for each channel, for each fft over time, for each intensity at
    // freq. fft is size FREQVIEW_SAMPLE_FFT_SCOPE_SIZE and there are numFft. an
    // fft covers FREQVIEW_SAMPLE_FFT_SIZE audio samples.
    std::vector<float> audioBufferFrequencies;
    // how many blocks of FREQVIEW_SAMPLE_FFT_SIZE samples
    // for this buffer
    int numFft;

    int trackIndex;

    juce::IIRFilter lowPassFilterLeft[SAMPLEPLAYER_MAX_FILTER_REPEAT];
    juce::IIRFilter lowPassFilterRight[SAMPLEPLAYER_MAX_FILTER_REPEAT];

    juce::IIRFilter highPassFilterLeft[SAMPLEPLAYER_MAX_FILTER_REPEAT];
    juce::IIRFilter highPassFilterRight[SAMPLEPLAYER_MAX_FILTER_REPEAT];

    void applyFilters(const juce::AudioSourceChannelInfo &bufferToFill);
};

#endif // DEF_SAMPLEPLAYER_HPP