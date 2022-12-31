#include "SamplePlayer.h"
#include "ColorPalette.h"

#include "Config.h"
#include <iterator>

int SamplePlayer::lastUsedColor = 0;

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
    // pick a random color
    colour = colourPalette[lastUsedColor];
    lastUsedColor = (lastUsedColor + 1)%colourPalette.size();
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

void SamplePlayer::setBuffer(BufferPtr targetBuffer, juce::dsp::FFT &fft) {
    // get lock and change buffer
    const juce::SpinLock::ScopedLockType lock (playerMutex);
    audioBufferRef = targetBuffer;

    int numChannels = targetBuffer->getAudioSampleBuffer()->getNumChannels();
    int numSamples = targetBuffer->getAudioSampleBuffer()->getNumSamples();
    int numFft = numSamples / FREQVIEW_SAMPLE_FFT_SIZE;

    // reset sample length
    bufferStart = 0;
    bufferEnd = numSamples-1;
    isSampleSet = true;

    // allocate the buffer where the fft result will be stored
    audioBufferFrequencies.clear();
    audioBufferFrequencies.reserve(numChannels*(numFft*FREQVIEW_SAMPLE_FFT_SIZE));

    // allocate a buffer to perform a fft (double size of fft)
    std::vector<float> inputOutputData((FREQVIEW_SAMPLE_FFT_SIZE)<<1, 0.0f);

    float const * audioBufferPosition;

    // TODO: skip fft for one in something samples blocks.
    // We should trade some perf/memory for a lower time resolution.

    // TODO: we currently ignore the remaining samples after all
    // 1024 (FREQVIEW_SAMPLE_FFT_SIZE) blocks are processed.
    // This represent maybe 1 / 40 th of a second of signal that are ignored.
    // We should process it in the future.

    // for each channel
    for(size_t i=0; i<numChannels; i++) {
        audioBufferPosition = targetBuffer->getAudioSampleBuffer()->getReadPointer(i);
        // iterate over buffers of 1024 samples
        for(size_t j=0; j<numFft; j++) {
            // fill the input buffer with zeros.
            // NOTE: is it really necessary ? 
            std::fill(inputOutputData.begin(), inputOutputData.end(), 0.0f);
            // copy input data and increment position in audio buffer
            // NOTE: I apologize for using C inside C++, please forgive my ignorance
            memcpy(
                &inputOutputData[0],
                audioBufferPosition,
                FREQVIEW_SAMPLE_FFT_SIZE*sizeof(float)
            );
            audioBufferPosition += FREQVIEW_SAMPLE_FFT_SIZE;
            // do the actual fft processing
            fft.performFrequencyOnlyForwardTransform(&inputOutputData[0]);
            // copy back the results
            audioBufferFrequencies.insert(
                audioBufferFrequencies.end(),
                inputOutputData.begin(),
                inputOutputData.begin()+FREQVIEW_SAMPLE_FFT_SIZE
            );
        }
    }
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
    return (bufferEnd-bufferStart)+1;
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
        bufferEnd = bufferStart+length-1;
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
        bufferPosition += bufferToFill.numSamples;
        position += bufferToFill.numSamples;
        return;
    }

    // fetch audio buffer data
    auto* currentAudioSampleBuffer = retainedCurrentBuffer->getAudioSampleBuffer();
    auto numInputChannels = currentAudioSampleBuffer->getNumChannels();
    auto numOutputChannels = bufferToFill.buffer->getNumChannels();
    int64_t outputSamplesRemaining = bufferToFill.numSamples;
    int64_t outputSamplesOffset = 0;

    // NOTE: for now, let's consider bufferStart will always be zero.
    // We start simple for now, and will only introduce bugs later :D 

    // update audio buffer position
    bufferPosition = position - editingPosition;

    // play nothing if sample is not playing
    if ((bufferPosition+outputSamplesRemaining) < 0 || bufferPosition > (bufferEnd-bufferStart)) {
        bufferToFill.clearActiveBufferRegion();
        bufferPosition += bufferToFill.numSamples;
        position += bufferToFill.numSamples;
        return;
    }

    // pad beginning to start copying after the buffers starts
    if(bufferPosition<0) {
        outputSamplesOffset = -bufferPosition;
        outputSamplesRemaining -= -bufferPosition;
    }

    // send audio buffer data
    while (outputSamplesRemaining > 0)
    {
        // decide on how many samples to copy
        auto bufferSamplesRemaining = (bufferEnd-bufferStart)+ 1 - bufferPosition - outputSamplesOffset;
        auto samplesThisTime = juce::jmin (outputSamplesRemaining, bufferSamplesRemaining);

        if(samplesThisTime==0) {
            break;
        }

        // copy audio for each channel
        for (auto channel = 0; channel < numOutputChannels; ++channel)
        {
            bufferToFill.buffer->copyFrom (channel,
                                            bufferToFill.startSample + outputSamplesOffset,
                                            *currentAudioSampleBuffer,
                                            channel % numInputChannels,
                                            bufferPosition + outputSamplesOffset,
                                            samplesThisTime);
        }

        outputSamplesRemaining -= samplesThisTime;
        outputSamplesOffset += samplesThisTime;
    }

    bufferPosition += bufferToFill.numSamples;
    position += bufferToFill.numSamples;
}

int64_t SamplePlayer::getEditingPosition() const {
    return editingPosition;
}