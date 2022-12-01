#ifndef DEF_SAMPLEPLAYER_HPP
#define DEF_SAMPLEPLAYER_HPP

// CMake builds don't use an AppConfig.h, so it's safe to include juce module
// headers directly. If you need to remain compatible with Projucer-generated
// builds, and have called `juce_generate_juce_header(<thisTarget>)` in your
// CMakeLists.txt, you could `#include <JuceHeader.h>` here instead, to make all
// your module headers visible.
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include <atomic>

class SamplePlayer: public juce::PositionableAudioSource
{
public:
    SamplePlayer(int64_t position);
    ~SamplePlayer();
    // this tells the SamplePlayer which audio buffer to use
    setBuffer(ReferenceCountedBuffer::Ptr targetBuffer);
    
    // inherited from PositionableAudioSource
    juce::int64 getNextReadPosition() override;
    void setNextReadPosition(juce::int64) override;
    juce::int64 getTotalLength() override;
    bool isLooping() override;

    
    // These two functions allocate a new SamplePlayer pointing
    // to the same audio buffer. It sets the current one to one side
    // of the limit, and the new one to the other.

    // will split the sample in two at a frquency provided
    SamplePlayer* split(float frequencyLimitHz);
    // will split the sample in two at the time provided
    SamplePlayer* split(juce::int64 positionLimit);
    
    // TODO: add all editing functions

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReferenceCountedBuffer)
    std::atomic<int64_t> editingPosition;
    std::atomic<int64_t> bufferPosition;
    std::atomic<int64_t> bufferStart;
    std::atomic<int64_t> bufferEnd;
}

#endif // DEF_SAMPLEPLAYER_HPP