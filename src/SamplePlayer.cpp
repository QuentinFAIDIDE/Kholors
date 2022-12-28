#include "SamplePlayer.h"
#include "ColorPalette.h"

#include <random>

SamplePlayer::SamplePlayer(int64_t position):
    editingPosition(position),
    bufferPosition(0),
    bufferStart(0),
    bufferEnd(0),
    position(0),
    lowPassFreq(44100),
    highPassFreq(0),
    isSampleSet(false)
{
    // initialize randommness to pick a colour
    // TODO: do not use one random_device per number
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, colourPalette.size());

    // pick a random color
    colour = colourPalette[distrib(gen)];
}

SamplePlayer::~SamplePlayer() {
    // TODO
}

juce::Colour& SamplePlayer::getColor() {
    return colour;
}

void SamplePlayer::setColor(int colorId) {
    colour = colourPalette[colorId%colourPalette.size()];
}

void SamplePlayer::setBuffer(BufferPtr targetBuffer) {
    // get lock and change buffer
    const juce::SpinLock::ScopedLockType lock (playerMutex);
    audioBufferRef = targetBuffer;
    // reset sample length
    bufferStart = 0;
    bufferEnd = targetBuffer->getAudioSampleBuffer()->getNumSamples();
    isSampleSet = true;
}

// inherited from PositionableAudioSource
juce::int64 SamplePlayer::getNextReadPosition() const {
    return position;
}

void SamplePlayer::setNextReadPosition(juce::int64 p) {
    position = p;
}

// length of entire buffer
juce::int64 SamplePlayer::getTotalLength() const {
    return bufferEnd-bufferStart;
}

bool SamplePlayer::isLooping() const {
    // Unsopported
    return false;
}

// move the sample to a new track position
void SamplePlayer::move(juce::int64 newPosition) {
    editingPosition = newPosition;
}

// set the length up to which reading the buffer
void SamplePlayer::setLength(juce::int64 length) {
    const juce::SpinLock::ScopedLockType lock (playerMutex);
    if (bufferStart+length < audioBufferRef->getAudioSampleBuffer()->getNumSamples()) {
        bufferEnd = bufferStart+length;
    } else {
        bufferEnd = audioBufferRef->getAudioSampleBuffer()->getNumSamples();
    }
}

// get the length up to which the buffer is readead
juce::int64 SamplePlayer::getLength() const {
    return bufferEnd-bufferStart;
}

// set the shift for the buffer reading start position.
// Shift parameter is the shift from audio buffer beginning.
void SamplePlayer::setBufferShift(juce::int64 shift) {
    const juce::SpinLock::ScopedLockType lock (playerMutex);
    // NOTE: Future feature, won't play with shift !
    
    // only change if the buffer can actuallydo it
    if(shift < bufferEnd) {
        bufferStart = shift;
    }
}

// get the shift of the buffer shift
juce::int64 SamplePlayer::getBufferShift() const {
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
    isSampleSet = false;
    audioBufferRef = BufferPtr(nullptr);
}

void SamplePlayer::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) {
    // return buffer data like in:
    // https://docs.juce.com/master/tutorial_looping_audio_sample_buffer_advanced.html

    // safely get the current buffer
    auto retainedCurrentBuffer = [&]() -> BufferPtr
    {
        // get scoped lock
        const juce::SpinLock::ScopedTryLockType lock (playerMutex);

        if (lock.isLocked())
            return audioBufferRef;

        return nullptr;
    }();

    // return empty buffer if no buffer is set
    if (retainedCurrentBuffer == nullptr)
    {
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    // fetch audio buffer data
    auto* currentAudioSampleBuffer = retainedCurrentBuffer->getAudioSampleBuffer();
    auto numInputChannels = currentAudioSampleBuffer->getNumChannels();
    auto numOutputChannels = bufferToFill.buffer->getNumChannels();
    int64_t outputSamplesRemaining = bufferToFill.numSamples;
    auto outputSamplesOffset = 0;

    // NOTE: for now, let's consider bufferStart will always be zero.
    // We start simple for now, and will only introduce bugs later :D 

    // update audio buffer position
    bufferPosition = position - editingPosition;

    // play nothing if sample is not playing
    if ((bufferPosition+outputSamplesRemaining) < 0 || bufferPosition > (bufferEnd-bufferStart)) {
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    // send audio buffer data
    while (outputSamplesRemaining > 0)
    {
        // decide on how many samples to copy
        auto bufferSamplesRemaining = (bufferEnd-bufferStart) - bufferPosition;
        auto samplesThisTime = juce::jmin (outputSamplesRemaining, bufferSamplesRemaining);

        // copy audio for each channel
        for (auto channel = 0; channel < numOutputChannels; ++channel)
        {
            bufferToFill.buffer->copyFrom (channel,
                                            bufferToFill.startSample + outputSamplesOffset,
                                            *currentAudioSampleBuffer,
                                            channel % numInputChannels,
                                            bufferPosition,
                                            samplesThisTime);
        }

        outputSamplesRemaining -= samplesThisTime;
        outputSamplesOffset += samplesThisTime;
        bufferPosition += samplesThisTime;
        position += samplesThisTime;
    }
}

int64_t SamplePlayer::getEditingPosition() const {
    return editingPosition;
}