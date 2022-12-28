#ifndef DEF_SAMPLEPLAYER_HPP
#define DEF_SAMPLEPLAYER_HPP

// CMake builds don't use an AppConfig.h, so it's safe to include juce module
// headers directly. If you need to remain compatible with Projucer-generated
// builds, and have called `juce_generate_juce_header(<thisTarget>)` in your
// CMakeLists.txt, you could `#include <JuceHeader.h>` here instead, to make all
// your module headers visible.
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include "ReferenceCountedBuffer.h"

#include <atomic>

class SamplePlayer: public juce::PositionableAudioSource
{
public:
    SamplePlayer(int64_t position);
    ~SamplePlayer();
    // this tells the SamplePlayer which audio buffer to use
    void setBuffer(BufferPtr targetBuffer);

    // inherited from PositionableAudioSource
    juce::int64 getNextReadPosition() const override;
    void setNextReadPosition(juce::int64) override;
    juce::int64 getTotalLength() const override;
    bool isLooping() const override;


    // AudioSource inherited functions
    void prepareToPlay(int, double) override;
    void releaseResources() override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo&) override;

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
    // create and move a duplicate (uses same underlying audio buffer)
    SamplePlayer* createDuplicate(juce::int64);
    // will split the sample in two at a frquency provided
    // (returns new other half)
    SamplePlayer* split(float frequencyLimitHz);
    // will split the sample in two at the time provided
    // (returns new other half)
    SamplePlayer* split(juce::int64 positionLimit);

    // a lock to switch buffers and safely read in message thread (gui)
    juce::SpinLock playerMutex;
    // helpers to read graphical properties
    int64_t getEditingPosition() const;
    juce::Colour& getColor();
    void setColor(int colorId);

    // TODO: getters for stereo low and high pass

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SamplePlayer)
    std::atomic<int64_t> editingPosition;
    std::atomic<int64_t> bufferPosition;
    std::atomic<int64_t> bufferStart;
    std::atomic<int64_t> bufferEnd;
    // this position is the one shared with all tracks (cursor pos)
    std::atomic<int64_t> position;
    std::atomic<float> lowPassFreq;
    std::atomic<float> highPassFreq;
    BufferPtr audioBufferRef;
    bool isSampleSet;
    juce::Colour colour;

    // index of the last used color from colorPalette
    static int lastUsedColor;
};

#endif // DEF_SAMPLEPLAYER_HPP