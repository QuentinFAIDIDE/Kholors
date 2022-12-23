#include "SamplePlayer.h"

SamplePlayer::SamplePlayer(int64_t position):
    editingPosition(position),
    bufferPosition(0),
    bufferStart(0),
    bufferEnd(0),
    position(0),
    lowPassFreq(44100),
    highPassFreq(0),
    isSampleSet(false),
{
    // TODO
}

SamplePlayer::~SamplePlayer() {
    // TODO
}

void SamplePlayer::setBuffer(BufferPtr targetBuffer) {
    audioBufferRef = targetBuffer;
    // reset sample length
    bufferStart = 0;
    bufferEnd = targetBuffer.buffer.getNumSamples();
    isSampleSet = true;
}

// inherited from PositionableAudioSource
juce::int64 SamplePlayer::getNextReadPosition() {
    return position;
}

void SamplePlayer::setNextReadPosition(juce::int64 p) {
    position = p;
}

// length of entire buffer
juce::int64 SamplePlayer::getTotalLength() {
    return bufferEnd-bufferStart;
}

bool SamplePlayer::isLooping() {
    // Unsopported
    return false;
}

// move the sample to a new track position
void SamplePlayer::move(juce::int64 newPosition) {
    editingPosition = newPosition;
}

// set the length up to which reading the buffer
void SamplePlayer::setLength(juce::int64 length) {
    if (bufferStart+length < targetBuffer.buffer.getNumSamples()) {
        bufferEnd = bufferStart+length;
    } else {
        bufferEnd = targetBuffer.buffer.getNumSamples();
    }
}

// get the length up to which the buffer is readead
void SamplePlayer::getLength(juce::int64) {
    return bufferEnd-bufferStart;
}

// set the shift for the buffer reading start position.
// Shift parameter is the shift from audio buffer beginning.
void SamplePlayer::setBufferShift(juce::int64 shift) {
    // only change if the buffer can actuallydo it
    if(shift < bufferEnd) {
        bufferStart = shift;
    }
}

// get the shift of the buffer shift
juce::int64 SamplePlayer::getBufferShift() {
    return bufferStart;
}

// create and move a duplicate (uses same underlying audio buffer)
SamplePlayer* SamplePlayer::createDuplicate(juce::int64 newPosition) {
    // TODO
    return nullptr;
}

// will split the sample in two at a frquency provided
// (returns new other half)
SamplePlayer* SamplePlayer::split(float frequencyLimitHz) {
    // TODO
    return nullptr;
}

// will split the sample in two at the time provided
// (returns new other half)
SamplePlayer* SamplePlayer::split(juce::int64 positionLimit) {
    // TODO
    return nullptr;
}

void SamplePlayer::prepareToPlay(int samplesPerBlockExpected, double sampleRate) {
    // TODO
}

void SamplePlayer::releaseResources() {
    isSampleSet = false
    audioBufferRef = BufferPtr(nullptr)
}

void SamplePlayer::getNextAudioBlock(const AudioSourceChannelInfo& bufferToFill) {
    // TODO: return buffer data like https://docs.juce.com/master/tutorial_looping_audio_sample_buffer_advanced.html
}