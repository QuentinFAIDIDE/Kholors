#include "SamplePlayer.h"
#include "UnitConverter.h"

#include <iterator>

int SamplePlayer::maxFilterFreq = (AUDIO_FRAMERATE >> 1) - 1;

SamplePlayer::SamplePlayer(int64_t position)
    : editingPosition(position), bufferInitialPosition(0), bufferStart(0), bufferEnd(0), position(0),
      lowPassFreq(maxFilterFreq), highPassFreq(0), isSampleSet(false), numFft(0), trackIndex(-1)
{
    setLowPassFreq(lowPassFreq);
    setHighPassFreq(highPassFreq);
}

SamplePlayer::~SamplePlayer()
{
    // TODO
}

bool SamplePlayer::hasBeenInitialized() const
{
    return isSampleSet;
}

void SamplePlayer::setBuffer(BufferPtr targetBuffer, juce::dsp::FFT &fft)
{
    // get lock and change buffer
    const juce::SpinLock::ScopedLockType lock(playerMutex);
    audioBufferRef = targetBuffer;

    int numChannels = targetBuffer->getAudioSampleBuffer()->getNumChannels();
    int numSamples = targetBuffer->getAudioSampleBuffer()->getNumSamples();
    numFft = numSamples / (FREQVIEW_SAMPLE_FFT_SIZE);

    // reset sample length
    bufferStart = 0;
    bufferEnd = numSamples - 1;
    isSampleSet = true;

    // allocate the buffer where the fft result will be stored
    audioBufferFrequencies.clear();
    audioBufferFrequencies.resize(numChannels * numFft * FREQVIEW_SAMPLE_FFT_SCOPE_SIZE);
    std::fill(audioBufferFrequencies.begin(), audioBufferFrequencies.end(), 0.0f);

    // allocate a buffer to perform a fft (double size of fft)
    std::vector<float> inputOutputData((FREQVIEW_SAMPLE_FFT_SIZE) << 1, 0.0f);

    float const *audioBufferData;
    int audioBufferPosition;

    // magic windowing function to reduce spectral leakage in fft
    juce::dsp::WindowingFunction<float> window(FREQVIEW_SAMPLE_FFT_SIZE, juce::dsp::WindowingFunction<float>::hann);

    // TODO: we currently ignore the remaining samples after all
    // 1024 (FREQVIEW_SAMPLE_FFT_SIZE) blocks are processed.
    // This represent maybe 1 / 40 th of a second of signal that are ignored.
    // We should process it in the future.

    // for each channel
    for (size_t i = 0; i < numChannels; i++)
    {
        audioBufferData = targetBuffer->getAudioSampleBuffer()->getReadPointer(i);
        audioBufferPosition = 0;

        auto channelFftIndex = (i * numFft);

        // iterate over buffers of 1024 samples
        for (size_t j = 0; j < numFft; j++)
        {
            // fill the input buffer with zeros.
            // NOTE: is it really necessary ?
            std::fill(inputOutputData.begin(), inputOutputData.end(), 0.0f);
            // copy input data and increment position in audio buffer
            // NOTE: I apologize for using C inside C++, please forgive my ignorance
            memcpy(&inputOutputData[0], &audioBufferData[audioBufferPosition],
                   FREQVIEW_SAMPLE_FFT_SIZE * sizeof(float));
            audioBufferPosition += FREQVIEW_SAMPLE_FFT_SIZE;
            // do the actual fft processing
            window.multiplyWithWindowingTable(&inputOutputData[0], FREQVIEW_SAMPLE_FFT_SIZE);
            fft.performFrequencyOnlyForwardTransform(&inputOutputData[0], true);

            // fft index in the destination storage
            auto fftIndex = ((channelFftIndex + j) * FREQVIEW_SAMPLE_FFT_SCOPE_SIZE);

            // convert the result into decibels
            for (size_t k = 0; k < (FREQVIEW_SAMPLE_FFT_SIZE >> 1); k++)
            {
                inputOutputData[k] = UnitConverter::fftToDb(inputOutputData[k]);
            }
            // copy back the results
            for (size_t k = 0; k < FREQVIEW_SAMPLE_FFT_SCOPE_SIZE; k++)
            {
                // NOTE: The relevant frequency amplitude data is half the fft size, so
                // a quarter of our inputdata array example idea
                // https://docs.juce.com/master/tutorial_spectrum_analyser.html

                // map the index to magnify important frequencies
                auto logIndexFft = UnitConverter::magnifyFftIndex(k);

                audioBufferFrequencies[fftIndex + k] = inputOutputData[logIndexFft];
            }
        }
    }
}

int SamplePlayer::getNumFft() const
{
    return numFft;
}

int SamplePlayer::getBufferNumChannels() const
{
    // note: WE DO NOT CHECK THAT AUDIOBUFFERREF IS SET !
    // beware of segfaults if you play with SamplePlayer
    // outside of the tracks SampleManager list!
    return audioBufferRef->getAudioSampleBuffer()->getNumChannels();
}

std::string SamplePlayer::getFileName()
{
    if (audioBufferRef == nullptr)
    {
        return "None";
    }
    else
    {
        return audioBufferRef->getName();
    }
}

std::vector<float> &SamplePlayer::getFftData()
{
    return audioBufferFrequencies;
}

// inherited from PositionableAudioSource
juce::int64 SamplePlayer::getNextReadPosition() const
{
    return position;
}

void SamplePlayer::setNextReadPosition(juce::int64 p)
{
    position = p;
}

// length of entire buffer
juce::int64 SamplePlayer::getTotalLength() const
{
    return audioBufferRef->getAudioSampleBuffer()->getNumSamples();
}

bool SamplePlayer::isLooping() const
{
    // Unsopported
    return false;
}

// move the sample to a new track position
void SamplePlayer::move(juce::int64 newPosition)
{
    editingPosition = newPosition;
}

// set the length up to which reading the buffer
void SamplePlayer::setLength(juce::int64 length)
{
    const juce::SpinLock::ScopedLockType lock(playerMutex);
    if (bufferStart + length < audioBufferRef->getAudioSampleBuffer()->getNumSamples())
    {
        bufferEnd = bufferStart + length - 1;
    }
    else
    {
        bufferEnd = audioBufferRef->getAudioSampleBuffer()->getNumSamples() - 1;
    }
}

// get the length up to which the buffer is readead
juce::int64 SamplePlayer::getLength() const
{
    return (bufferEnd - bufferStart) + 1;
}

int SamplePlayer::getBufferStart() const
{
    return bufferStart;
}

int SamplePlayer::getBufferEnd() const
{
    return bufferEnd;
}

// tryMovingStart will try to shift the beginning of played buffer section from
// desired number of frames. It return the actual shift performed.
int SamplePlayer::tryMovingStart(int desiredShift)
{
    if (desiredShift == 0)
    {
        return 0;
    }

    int actualChange;
    if (desiredShift < 0)
    {
        actualChange = -juce::jmin(bufferStart, -desiredShift);
    }
    else
    {
        actualChange = juce::jmin((int)getLength() - SAMPLEPLAYER_MIN_FRAME_SIZE, desiredShift);
    }

    setBufferShift(getBufferShift() + actualChange);
    move(getEditingPosition() + actualChange);
    return actualChange;
}

// tryMovingStart will try to change the length of played buffer section from
// desired number of frames. It return the actual shift performed.
int SamplePlayer::tryMovingEnd(int desiredShift)
{
    if (desiredShift == 0)
    {
        return 0;
    }

    int actualChange;
    if (desiredShift > 0)
    {
        int lastBufferIndexAvailable = (getTotalLength() - 1);
        actualChange = juce::jmin(lastBufferIndexAvailable - bufferEnd, desiredShift);
    }
    else
    {
        actualChange = -juce::jmin((int)getLength() - SAMPLEPLAYER_MIN_FRAME_SIZE, -desiredShift);
    }

    setLength(getLength() + actualChange);
    return actualChange;
}

// set the shift for the buffer reading start position.
// Shift parameter is the shift from audio buffer beginning.
void SamplePlayer::setBufferShift(juce::int64 shift)
{
    const juce::SpinLock::ScopedLockType lock(playerMutex);
    // only change if the buffer can actuallydo it
    if (shift < bufferEnd - SAMPLEPLAYER_MIN_FRAME_SIZE)
    {
        bufferStart = shift;
    }
}

// get the shift of the buffer shift
juce::int64 SamplePlayer::getBufferShift() const
{
    return bufferStart;
}

// create and move a duplicate (uses same underlying audio buffer)
SamplePlayer *SamplePlayer::createDuplicate(juce::int64 newPosition, juce::dsp::FFT &fft)
{
    SamplePlayer *duplicate = new SamplePlayer(newPosition);
    duplicate->setBuffer(audioBufferRef, fft);
    duplicate->setBufferShift(bufferStart);
    duplicate->setLength(getLength());
    return duplicate;
}

// will split the sample in two at a frquency provided
// (returns new other half)
SamplePlayer *SamplePlayer::split(float frequencyLimitHz)
{
    // TODO
    return nullptr;
}

// will split the sample in two at the time provided
// (returns new other half)
SamplePlayer *SamplePlayer::split(juce::int64 positionLimit)
{
    // TODO
    return nullptr;
}

void SamplePlayer::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    // TODO
}

void SamplePlayer::releaseResources()
{
    isSampleSet = false;
    audioBufferRef = BufferPtr(nullptr);
}

void SamplePlayer::getNextAudioBlock(const juce::AudioSourceChannelInfo &bufferToFill)
{
    // return buffer data like in:
    // https://docs.juce.com/master/tutorial_looping_audio_sample_buffer_advanced.html

    // safely get the current buffer
    auto retainedCurrentBuffer = [&]() -> BufferPtr {
        // get scoped lock
        const juce::SpinLock::ScopedTryLockType lock(playerMutex);

        if (lock.isLocked())
            return audioBufferRef;

        return nullptr;
    }();

    // return cleared buffer if no buffer is set or if lock failed to be locked.
    if (retainedCurrentBuffer == nullptr)
    {
        bufferToFill.clearActiveBufferRegion();
        applyFilters(bufferToFill);
        position += bufferToFill.numSamples;
        return;
    }

    // samplePlayer audio buffer data
    auto *currentAudioSampleBuffer = retainedCurrentBuffer->getAudioSampleBuffer();
    auto numInputChannels = currentAudioSampleBuffer->getNumChannels();
    auto numOutputChannels = bufferToFill.buffer->getNumChannels();
    auto outputSamplesRemaining = bufferToFill.numSamples;
    // how many samples have we already read ? (in this call to getNextAudioBlock)
    int64_t outputSamplesOffset = 0;

    // set position relative to bufferStart
    bufferInitialPosition = position - editingPosition;

    // play nothing if sample is not playing
    if ((bufferInitialPosition + outputSamplesRemaining) < 0 || bufferInitialPosition > bufferEnd - bufferStart)
    {
        bufferToFill.clearActiveBufferRegion();
        position += bufferToFill.numSamples;
        applyFilters(bufferToFill);
        return;
    }

    int64_t skippedSamples = 0;
    // pad beginning to start copying after the buffers starts
    if (bufferInitialPosition < 0)
    {
        skippedSamples = -bufferInitialPosition;
        // clear region untill the sample plays
        bufferToFill.buffer->clear(bufferToFill.startSample, skippedSamples);
        // move output cursors to the beginning of the sample
        outputSamplesOffset += skippedSamples;
        outputSamplesRemaining -= skippedSamples;
    }

    // send audio buffer data
    while (outputSamplesRemaining > 0)
    {
        // decide on how many samples to copy
        int bufferSamplesRemaining = bufferEnd - (bufferStart + bufferInitialPosition + outputSamplesOffset);
        int samplesThisTime = juce::jmin(outputSamplesRemaining, bufferSamplesRemaining);

        if (samplesThisTime <= 0)
        {
            break;
        }

        // copy audio for each channel
        for (auto channel = 0; channel < numOutputChannels; ++channel)
        {
            bufferToFill.buffer->copyFrom(channel, bufferToFill.startSample + outputSamplesOffset,
                                          *currentAudioSampleBuffer, channel % numInputChannels,
                                          bufferStart + bufferInitialPosition + outputSamplesOffset, samplesThisTime);
        }

        outputSamplesRemaining -= samplesThisTime;
        outputSamplesOffset += samplesThisTime;
    }

    // if we broke out of loop because sample buffer was exhausted, clear the
    // remaining part
    if (outputSamplesRemaining > 0)
    {
        bufferToFill.buffer->clear(bufferToFill.startSample + outputSamplesOffset, outputSamplesRemaining);
    }

    // update the global track position stored in the samplePlayer
    position += bufferToFill.numSamples;

    applyFilters(bufferToFill);
}

int64_t SamplePlayer::getEditingPosition() const
{
    return editingPosition;
}

void SamplePlayer::setTrackIndex(int id)
{
    trackIndex = id;
}

int SamplePlayer::getTrackIndex()
{
    return trackIndex;
}

void SamplePlayer::setLowPassFreq(int freq)
{
    lowPassFreq = freq;

    if (lowPassFreq < 0)
    {
        lowPassFreq = 0;
    }

    if (lowPassFreq >= maxFilterFreq)
    {
        lowPassFreq = maxFilterFreq;
        for (size_t i = 0; i < SAMPLEPLAYER_MAX_FILTER_REPEAT; i++)
        {
            lowPassFilterLeft[i].makeInactive();
            lowPassFilterRight[i].makeInactive();
        }
        return;
    }

    auto coefs = juce::IIRCoefficients::makeLowPass(AUDIO_FRAMERATE, lowPassFreq);
    for (size_t i = 0; i < SAMPLEPLAYER_MAX_FILTER_REPEAT; i++)
    {
        lowPassFilterLeft[i].setCoefficients(coefs);
        lowPassFilterRight[i].setCoefficients(coefs);
    }
}

void SamplePlayer::setHighPassFreq(int freq)
{
    highPassFreq = freq;

    if (highPassFreq > maxFilterFreq)
    {
        highPassFreq = maxFilterFreq;
    }

    if (highPassFreq <= 0)
    {
        highPassFreq = 0;
        for (size_t i = 0; i < SAMPLEPLAYER_MAX_FILTER_REPEAT; i++)
        {
            highPassFilterLeft[i].makeInactive();
            highPassFilterRight[i].makeInactive();
        }
        return;
    }

    auto coefs = juce::IIRCoefficients::makeHighPass(AUDIO_FRAMERATE, highPassFreq);
    for (size_t i = 0; i < SAMPLEPLAYER_MAX_FILTER_REPEAT; i++)
    {
        highPassFilterLeft[i].setCoefficients(coefs);
        highPassFilterRight[i].setCoefficients(coefs);
    }
}

void SamplePlayer::applyFilters(const juce::AudioSourceChannelInfo &bufferToFill)
{
    for (size_t channel = 0; channel < bufferToFill.buffer->getNumChannels(); ++channel)
    {

        juce::IIRFilter *highPassFilters, *lowPassFilters;
        if (channel % 2 == 0)
        {
            highPassFilters = highPassFilterLeft;
            lowPassFilters = lowPassFilterLeft;
        }
        else
        {
            highPassFilters = highPassFilterRight;
            lowPassFilters = lowPassFilterRight;
        }

        float *audioSamples = bufferToFill.buffer->getWritePointer(channel);
        for (size_t i = 0; i < SAMPLEPLAYER_MAX_FILTER_REPEAT; i++)
        {
            highPassFilters[i].processSamples(audioSamples + bufferToFill.startSample * sizeof(float),
                                              bufferToFill.numSamples);
            lowPassFilters[i].processSamples(audioSamples + bufferToFill.startSample * sizeof(float),
                                             bufferToFill.numSamples);
        }
    }
}

float SamplePlayer::getLowPassFreq()
{
    return lowPassFreq;
}

float SamplePlayer::getHighPassFreq()
{
    return highPassFreq;
}