#include "SamplePlayer.h"
#include "AudioFilesBufferStore.h"
#include "FftRunner.h"
#include "UnitConverter.h"

#include <complex>
#include <iterator>
#include <memory>
#include <stdexcept>

int SamplePlayer::maxFilterFreq = (AUDIO_FRAMERATE >> 1) - 1;

SamplePlayer::SamplePlayer(int64_t position)
    : editingPosition(position), bufferInitialPosition(0), bufferStart(0), bufferEnd(0), position(0),
      lowPassFreq(maxFilterFreq), highPassFreq(0), audioBufferRef(), isSampleSet(false), numFft(0)
{

    audioBufferFrequencies = std::make_shared<std::vector<float>>();

    lowPassRepeat = SAMPLEPLAYER_MAX_FILTER_REPEAT;
    highPassRepeat = SAMPLEPLAYER_MAX_FILTER_REPEAT;

    setDbGain(0.0f);
    setLowPassFreq(lowPassFreq);
    setHighPassFreq(highPassFreq);

    fadeInFrameLength = 0;
    fadeOutFrameLength = 0;

    setGainRamp(SAMPLEPLAYER_DEFAULT_FADE_IN_MS);
    setFadeInLength((AUDIO_FRAMERATE / 1000.0) * SAMPLEPLAYER_DEFAULT_FADE_IN_MS);
    setFadeOutLength((AUDIO_FRAMERATE / 1000.0) * SAMPLEPLAYER_DEFAULT_FADE_OUT_MS);
}

SamplePlayer::~SamplePlayer()
{
}

json SamplePlayer::toJSON()
{
    const juce::SpinLock::ScopedLockType lock(playerMutex);

    std::string filename = "";
    std::string filePath = "";
    int bufferLen = 0;

    if (audioBufferRef.data != nullptr)
    {
        filename = juce::File(audioBufferRef.fileFullPath).getFileName().toStdString();
        filePath = audioBufferRef.fileFullPath;
        bufferLen = audioBufferRef.data->getNumSamples();
    }

    json output = {{"file_name", filename},
                   {"file_path", filePath},
                   {"editing_position", editingPosition},
                   {"buffer_start", bufferStart},
                   {"buffer_end", bufferEnd},
                   {"audio_buffer_len", bufferLen}, // length of the audio buffer and not the local samplePlayer length
                   {"low_pass_freq", lowPassFreq},
                   {"high_pass_freq", highPassFreq},
                   {"gain", gainValue},
                   {"low_pass_repeat", lowPassRepeat},
                   {"high_pass_repeat", highPassRepeat},
                   {"fade_in_frame_len", fadeInFrameLength},
                   {"fade_out_frame_len", fadeOutFrameLength}};

    return output;
}

void SamplePlayer::setupFromJSON(json &stateToRestore)
{

    // NOTE: the sample buffer, ie file_name and file_path is already loaded
    // by mixbus

    std::string path;
    stateToRestore.at("file_path").get_to(path);

    // abort if the sample was not loaded
    if (audioBufferRef.data == nullptr)
    {
        throw std::runtime_error("A sample player had no buffer set when loaded!");
    }

    // throw an error if the sample loaded has a different size than the one
    // that was loaded at the moment the json was generated
    int desiredBufferLen;
    stateToRestore.at("audio_buffer_len").get_to(desiredBufferLen);
    if (desiredBufferLen != audioBufferRef.data->getNumSamples())
    {
        throw std::runtime_error("A sample player was loaded with a disk file that has different size: " + path);
    }

    stateToRestore.at("editing_position").get_to(editingPosition);

    int desiredBufferStartFrame, desiredBufferEndFrame;
    stateToRestore.at("buffer_start").get_to(desiredBufferStartFrame);
    stateToRestore.at("buffer_end").get_to(desiredBufferEndFrame);

    if (desiredBufferStartFrame < 0 || desiredBufferStartFrame >= desiredBufferLen)
    {
        throw std::runtime_error("A sample player was loaded with invalid buffer start frame");
    }

    if (desiredBufferEndFrame < 0 || desiredBufferEndFrame >= desiredBufferLen)
    {
        throw std::runtime_error("A sample player was loaded with invalid buffer end frame");
    }

    bufferStart = desiredBufferStartFrame;
    bufferEnd = desiredBufferEndFrame;

    float desiredLowPassFreq, desiredHighPassFreq;
    stateToRestore.at("low_pass_freq").get_to(desiredLowPassFreq);
    stateToRestore.at("high_pass_freq").get_to(desiredHighPassFreq);
    if (desiredLowPassFreq < 0 || desiredLowPassFreq > float(AUDIO_FRAMERATE >> 1) + 0.1f)
    {
        throw std::runtime_error("invalid low pass freq");
    }
    if (desiredHighPassFreq < 0 || desiredHighPassFreq > float(AUDIO_FRAMERATE >> 1) + 0.1f)
    {
        throw std::runtime_error("invalid high pass freq");
    }
    setLowPassFreq(desiredLowPassFreq);
    setHighPassFreq(desiredHighPassFreq);

    float desiredGain;
    int desiredLowPassRepeat, desiredHighPassRepeat;
    stateToRestore.at("gain").get_to(desiredGain);
    stateToRestore.at("low_pass_repeat").get_to(desiredLowPassRepeat);
    stateToRestore.at("high_pass_repeat").get_to(desiredHighPassRepeat);
    if (desiredLowPassRepeat <= 0 || desiredLowPassRepeat > SAMPLEPLAYER_MAX_FILTER_REPEAT)
    {
        throw std::runtime_error("low pass filter repeat out of range");
    }
    if (desiredHighPassRepeat <= 0 || desiredHighPassRepeat > SAMPLEPLAYER_MAX_FILTER_REPEAT)
    {
        throw std::runtime_error("high pass filter repeat out of range");
    }

    setLowPassRepeat(desiredLowPassRepeat);
    setHighPassRepeat(desiredHighPassRepeat);
    gainValue = desiredGain;

    float desiredFadeInLen, desiredFadeOutLen;
    stateToRestore.at("fade_in_frame_len").get_to(desiredFadeInLen);
    stateToRestore.at("fade_out_frame_len").get_to(desiredFadeOutLen);

    if (desiredFadeInLen + desiredFadeOutLen > getLength())
    {
        throw std::runtime_error("Trying to load a sample with an attack and release longer than sample.");
    }

    fadeInFrameLength = desiredFadeInLen;
    fadeOutFrameLength = desiredFadeOutLen;
}

void SamplePlayer::setDbGain(float gainDb)
{
    gainValue = juce::Decibels::decibelsToGain(gainDb);
}

float SamplePlayer::getDbGain()
{
    return juce::Decibels::gainToDecibels(gainValue);
}

void SamplePlayer::setGainRamp(float ms)
{

    int frameLength = ms * (float(AUDIO_FRAMERATE) / 1000.0);
    if (frameLength <= 0)
    {
        fadeInFrameLength = 0;
        fadeOutFrameLength = 0;
        return;
    }
    else if (ms > SAMPLEPLAYER_MAX_FADE_MS)
    {
        fadeInFrameLength = SAMPLEPLAYER_MAX_FADE_MS * (float(AUDIO_FRAMERATE) / 1000.0);
        fadeOutFrameLength = SAMPLEPLAYER_MAX_FADE_MS * (float(AUDIO_FRAMERATE) / 1000.0);
        return;
    }
    else if (2 * frameLength >= getLength())
    {
        fadeInFrameLength = getLength() >> 1;
        fadeOutFrameLength = getLength() >> 1;
    }
    else
    {
        fadeInFrameLength = frameLength;
        fadeOutFrameLength = frameLength;
    }
}

bool SamplePlayer::setFadeInLength(int length)
{
    if (length + fadeOutFrameLength >= getLength())
    {
        return false;
    }
    fadeInFrameLength = length;
    return true;
}

bool SamplePlayer::setFadeOutLength(int length)
{
    if (length + fadeInFrameLength >= getLength())
    {
        return false;
    }
    fadeOutFrameLength = length;
    return true;
}

int SamplePlayer::getFadeInLength()
{
    return fadeInFrameLength;
}

int SamplePlayer::getFadeOutLength()
{
    return fadeOutFrameLength;
}

bool SamplePlayer::hasBeenInitialized() const
{
    return isSampleSet;
}

void SamplePlayer::setBuffer(AudioFileBufferRef targetBuffer)
{
    // get lock and change buffer
    const juce::SpinLock::ScopedLockType lock(playerMutex);
    audioBufferRef = targetBuffer;

    int numSamples = targetBuffer.data->getNumSamples();
    numFft = FftRunner::getNumFftFromNumSamples(numSamples);

    // reset sample length
    bufferStart = 0;
    bufferEnd = numSamples - 1;
    isSampleSet = true;

    // set the fft data
    audioBufferFrequencies = targetBuffer.storedFftData;

    setGainRamp(SAMPLEPLAYER_DEFAULT_FADE_IN_MS);
    setFadeInLength((AUDIO_FRAMERATE / 1000.0) * SAMPLEPLAYER_DEFAULT_FADE_IN_MS);
    setFadeOutLength((AUDIO_FRAMERATE / 1000.0) * SAMPLEPLAYER_DEFAULT_FADE_OUT_MS);
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
    return audioBufferRef.data->getNumChannels();
}

std::string SamplePlayer::getFileName()
{
    if (audioBufferRef.data == nullptr)
    {
        return "None";
    }
    else
    {
        return juce::File(audioBufferRef.fileFullPath).getFileName().toStdString();
    }
}

std::shared_ptr<std::vector<float>> SamplePlayer::getFftData()
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
    if (!isSampleSet || audioBufferRef.data == nullptr)
    {
        return 0;
    }
    return audioBufferRef.data->getNumSamples();
}

bool SamplePlayer::isLooping() const
{
    // Unsopported
    return false;
}

// move the sample to a new track position
void SamplePlayer::move(juce::int64 newPosition)
{
    if (!isSampleSet || audioBufferRef.data == nullptr)
    {
        return;
    }
    editingPosition = newPosition;
}

// set the length up to which reading the buffer
void SamplePlayer::setLength(juce::int64 length)
{

    if (!isSampleSet || audioBufferRef.data == nullptr)
    {
        return;
    }

    if (length < SAMPLE_MIN_DURATION_FRAMES)
    {
        return;
    }

    const juce::SpinLock::ScopedLockType lock(playerMutex);
    if (bufferStart + length < audioBufferRef.data->getNumSamples())
    {
        bufferEnd = bufferStart + length - 1;
    }
    else
    {
        bufferEnd = audioBufferRef.data->getNumSamples() - 1;
    }

    checkGainRamps();
}

// get the length up to which the buffer is readead
juce::int64 SamplePlayer::getLength() const
{

    if (!isSampleSet || audioBufferRef.data == nullptr)
    {
        return 0;
    }

    return (bufferEnd - bufferStart) + 1;
}

int SamplePlayer::getBufferStart() const
{
    if (!isSampleSet || audioBufferRef.data == nullptr)
    {
        return 0;
    }

    return bufferStart;
}

int SamplePlayer::getBufferEnd() const
{
    if (!isSampleSet || audioBufferRef.data == nullptr)
    {
        return 0;
    }

    return bufferEnd;
}

// tryMovingStart will try to shift the beginning of played buffer section from
// desired number of frames. It return the actual shift performed.
int SamplePlayer::tryMovingStart(int desiredShift)
{

    if (!isSampleSet || audioBufferRef.data == nullptr)
    {
        return 0;
    }

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
        actualChange = juce::jmin((int)getLength() - SAMPLE_MIN_DURATION_FRAMES, desiredShift);
    }

    setBufferShift(getBufferShift() + actualChange);
    move(getEditingPosition() + actualChange);

    checkGainRamps();

    return actualChange;
}

// tryMovingStart will try to change the length of played buffer section from
// desired number of frames. It return the actual shift performed.
int SamplePlayer::tryMovingEnd(int desiredShift)
{

    if (!isSampleSet || audioBufferRef.data == nullptr)
    {
        return 0;
    }

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
        actualChange = -juce::jmin((int)getLength() - SAMPLE_MIN_DURATION_FRAMES, -desiredShift);
    }

    setLength(getLength() + actualChange);

    checkGainRamps();

    return actualChange;
}

// set the shift for the buffer reading start position.
// Shift parameter is the shift from audio buffer beginning.
void SamplePlayer::setBufferShift(juce::int64 shift)
{

    if (!isSampleSet || audioBufferRef.data == nullptr)
    {
        return;
    }

    const juce::SpinLock::ScopedLockType lock(playerMutex);
    // only change if the buffer can actuallydo it
    if (shift < bufferEnd - SAMPLE_MIN_DURATION_FRAMES)
    {
        bufferStart = shift;
    }

    checkGainRamps();
}

void SamplePlayer::checkGainRamps()
{
    fadeInFrameLength = fadeInFrameLength < 0 ? 0 : fadeInFrameLength;
    fadeOutFrameLength = fadeOutFrameLength < 0 ? 0 : fadeOutFrameLength;

    if (fadeInFrameLength + fadeOutFrameLength > getLength())
    {
        float fadeInProportion = float(fadeInFrameLength) / float(fadeInFrameLength + fadeOutFrameLength);
        fadeInFrameLength = float(getLength()) * fadeInProportion;
        fadeOutFrameLength = getLength() - fadeInFrameLength;
    }
}

// get the shift of the buffer shift
juce::int64 SamplePlayer::getBufferShift() const
{
    if (!isSampleSet || audioBufferRef.data == nullptr)
    {
        return 0;
    }

    return bufferStart;
}

// create and move a duplicate (uses same underlying audio buffer)
std::shared_ptr<SamplePlayer> SamplePlayer::createDuplicate(juce::int64 newPosition)
{

    if (!isSampleSet || audioBufferRef.data == nullptr)
    {
        return nullptr;
    }

    std::shared_ptr<SamplePlayer> duplicate = std::make_shared<SamplePlayer>(newPosition);
    duplicate->setBuffer(audioBufferRef);
    duplicate->setBufferShift(bufferStart);
    duplicate->setLength(getLength());
    duplicate->setLowPassFreq(lowPassFreq);
    duplicate->setHighPassFreq(highPassFreq);
    duplicate->setFadeInLength(fadeInFrameLength);
    duplicate->setFadeOutLength(fadeOutFrameLength);
    duplicate->setLowPassRepeat(lowPassRepeat);
    duplicate->setHighPassRepeat(highPassRepeat);
    duplicate->gainValue = gainValue;
    return duplicate;
}

std::shared_ptr<SamplePlayer> SamplePlayer::splitAtFrequency(float frequencyLimitHz)
{

    if (!isSampleSet || audioBufferRef.data == nullptr)
    {
        return nullptr;
    }

    float minFreq = addOnScreenAmountToFreq(highPassFreq, SAMPLEPLAYER_MIN_FREQ_DISTANCE_FACTOR);
    float maxFreq = addOnScreenAmountToFreq(lowPassFreq, -SAMPLEPLAYER_MIN_FREQ_DISTANCE_FACTOR);

    // check that lower part is large enought
    float highPassPositionRatio = UnitConverter::freqToPositionRatio(highPassFreq);
    float lowPassPositionRatio = UnitConverter::freqToPositionRatio(frequencyLimitHz);
    float diff = lowPassPositionRatio - highPassPositionRatio;
    if (diff < SAMPLEPLAYER_MIN_FREQ_DISTANCE_FACTOR)
    {
        return nullptr;
    }

    // same for top part
    highPassPositionRatio = UnitConverter::freqToPositionRatio(frequencyLimitHz);
    lowPassPositionRatio = UnitConverter::freqToPositionRatio(lowPassFreq);
    diff = lowPassPositionRatio - highPassPositionRatio;
    if (diff < SAMPLEPLAYER_MIN_FREQ_DISTANCE_FACTOR)
    {
        return nullptr;
    }

    if (frequencyLimitHz < minFreq || frequencyLimitHz > maxFreq)
    {
        return nullptr;
    }

    // we make a new sample that will be the low end part
    std::shared_ptr<SamplePlayer> duplicate = std::make_shared<SamplePlayer>(editingPosition);
    duplicate->setBuffer(audioBufferRef);
    duplicate->setBufferShift(bufferStart);
    duplicate->setLength(getLength());
    duplicate->setLowPassFreq(frequencyLimitHz);
    duplicate->setHighPassFreq(highPassFreq);
    duplicate->setFadeInLength(fadeInFrameLength);
    duplicate->setFadeOutLength(fadeOutFrameLength);
    duplicate->setLowPassRepeat(lowPassRepeat);
    duplicate->setHighPassRepeat(highPassRepeat);
    duplicate->gainValue = gainValue;

    // we are now the high end part
    setHighPassFreq(frequencyLimitHz);

    return duplicate;
}

std::shared_ptr<SamplePlayer> SamplePlayer::splitAtPosition(juce::int64 positionLimit)
{

    if (!isSampleSet || audioBufferRef.data == nullptr)
    {
        return nullptr;
    }

    if (positionLimit > getLength() - SAMPLE_MIN_DURATION_FRAMES)
    {
        return nullptr;
    }

    if (positionLimit < SAMPLE_MIN_DURATION_FRAMES)
    {
        return nullptr;
    }

    // we make a new sample that will be the last part
    std::shared_ptr<SamplePlayer> duplicate = std::make_shared<SamplePlayer>(editingPosition + positionLimit);
    duplicate->setBuffer(audioBufferRef);
    duplicate->setBufferShift(bufferStart + positionLimit);
    duplicate->setLength(getLength() - positionLimit);
    duplicate->setLowPassFreq(lowPassFreq);
    duplicate->setHighPassFreq(highPassFreq);
    duplicate->setFadeInLength(fadeInFrameLength);
    duplicate->setFadeOutLength(fadeOutFrameLength);
    duplicate->setLowPassRepeat(lowPassRepeat);
    duplicate->setHighPassRepeat(highPassRepeat);
    duplicate->gainValue = gainValue;

    // we are now the first part
    setLength(positionLimit);

    return duplicate;
}

float SamplePlayer::addOnScreenAmountToFreq(float freq, float screenProportion)
{
    // convert from frequency domain to on-screen domain
    float fftIndex = freq * (float(FFT_OUTPUT_NO_FREQS) / float(AUDIO_FRAMERATE >> 1));
    float storedFftDataIndex = UnitConverter::magnifyFftIndexInv(fftIndex);
    float textureIndex = UnitConverter::magnifyTextureFrequencyIndex(storedFftDataIndex);

    // now we can add the constant amount that matched on scren distance
    textureIndex = textureIndex + (screenProportion * float(FFT_STORAGE_SCOPE_SIZE));

    // now come back to frequency domain and return result
    storedFftDataIndex = UnitConverter::magnifyTextureFrequencyIndexInv(textureIndex);
    fftIndex = UnitConverter::magnifyFftIndex(storedFftDataIndex);
    return fftIndex * float(AUDIO_FRAMERATE >> 1) / float(FFT_OUTPUT_NO_FREQS);
}

void SamplePlayer::prepareToPlay(int, double)
{
    // TODO
}

void SamplePlayer::releaseResources()
{
    isSampleSet = false;
    audioBufferRef = AudioFileBufferRef();
}

void SamplePlayer::getNextAudioBlock(const juce::AudioSourceChannelInfo &bufferToFill)
{
    // return buffer data like in:
    // https://docs.juce.com/master/tutorial_looping_audio_sample_buffer_advanced.html

    // safely get the current buffer
    auto retainedCurrentBuffer = [&]() -> std::shared_ptr<juce::AudioSampleBuffer> {
        // get scoped lock
        const juce::SpinLock::ScopedTryLockType lock(playerMutex);

        if (lock.isLocked())
            return audioBufferRef.data;

        return nullptr;
    }();

    // return cleared buffer if no buffer is set or if lock failed to be locked.
    if (retainedCurrentBuffer == nullptr)
    {
        bufferToFill.clearActiveBufferRegion();
        applyFilters(bufferToFill);
        bufferToFill.buffer->applyGain(gainValue);
        position += bufferToFill.numSamples;
        return;
    }

    // samplePlayer audio buffer data
    auto *currentAudioSampleBuffer = retainedCurrentBuffer.get();
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
            applyGainFade(bufferToFill.buffer->getWritePointer(channel), bufferToFill.startSample + outputSamplesOffset,
                          samplesThisTime, bufferInitialPosition + outputSamplesOffset);
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
    bufferToFill.buffer->applyGain(gainValue);
}

void SamplePlayer::applyGainFade(float *data, int startIndex, int length, int startIndexLocalPosition)
{

    if (fadeInFrameLength <= 1 && fadeOutFrameLength <= 1)
    {
        return;
    }

    int stopIndexLocalPosition = startIndexLocalPosition + (length - 1);

    // these position are relative to bufferStart
    int fadeInStart = 0;
    int fadeInEnd = fadeInStart + (fadeInFrameLength - 1);
    int fadeOutStart = (getLength() - 1) - fadeOutFrameLength;
    int fadeOutEnd = getLength() - 1;

    bool includesFadeIn = startIndexLocalPosition < fadeInEnd && stopIndexLocalPosition >= fadeInStart;
    bool includesFadeOut = startIndexLocalPosition < fadeOutEnd && stopIndexLocalPosition >= fadeOutStart;
    if (!includesFadeIn && !includesFadeOut)
    {
        return;
    }

    if (!includesFadeIn && fadeOutFrameLength <= 1)
    {
        return;
    }

    if (!includesFadeOut && fadeInFrameLength <= 1)
    {
        return;
    }

    int currentLocalPosition = startIndexLocalPosition;
    int currentBlockPosition = startIndex;
    float factor;
    while (currentBlockPosition < startIndex + length)
    {

        // applies fade int
        if (includesFadeIn && fadeInFrameLength > 1 && currentLocalPosition >= fadeInStart &&
            currentLocalPosition <= fadeInEnd)
        {
            factor = float(currentLocalPosition - fadeInStart) / float(fadeInFrameLength - 1);
            data[currentBlockPosition] = factor * data[currentBlockPosition];
            currentLocalPosition++;
            currentBlockPosition++;
            continue;
        }

        // applies fade out
        if (includesFadeOut && fadeOutFrameLength > 1 && currentLocalPosition >= fadeOutStart &&
            currentLocalPosition <= fadeOutEnd)
        {
            factor = float(currentLocalPosition - fadeOutStart) / float(fadeOutFrameLength - 1);
            data[currentBlockPosition] = (1.0 - factor) * data[currentBlockPosition];
            currentLocalPosition++;
            currentBlockPosition++;
            continue;
        }

        currentLocalPosition++;
        currentBlockPosition++;
    }
}

int64_t SamplePlayer::getEditingPosition() const
{
    return editingPosition;
}

void SamplePlayer::setLowPassFreq(int freq)
{

    // check that it doesn't make us the sample too small
    float highPassPositionRatio = UnitConverter::freqToPositionRatio(highPassFreq);
    float lowPassPositionRatio = UnitConverter::freqToPositionRatio(freq);
    float diff = lowPassPositionRatio - highPassPositionRatio;

    if (diff < SAMPLEPLAYER_MIN_FREQ_DISTANCE_FACTOR)
    {
        return;
    }

    lowPassFreq = freq;

    if (lowPassFreq < highPassFreq)
    {
        lowPassFreq = highPassFreq;
    }

    if (lowPassFreq < SAMPLEPLAYER_MIN_FILTER_FREQ)
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

    // check that it doesn't make us the sample too small
    float highPassPositionRatio = UnitConverter::freqToPositionRatio(freq);
    float lowPassPositionRatio = UnitConverter::freqToPositionRatio(lowPassFreq);
    float diff = lowPassPositionRatio - highPassPositionRatio;

    if (diff < SAMPLEPLAYER_MIN_FREQ_DISTANCE_FACTOR)
    {
        return;
    }

    highPassFreq = freq;

    if (highPassFreq > maxFilterFreq)
    {
        highPassFreq = maxFilterFreq;
    }

    if (highPassFreq > lowPassFreq)
    {
        highPassFreq = lowPassFreq;
    }

    if (highPassFreq <= SAMPLEPLAYER_MIN_FILTER_FREQ)
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
    for (size_t channel = 0; channel < (size_t)bufferToFill.buffer->getNumChannels(); ++channel)
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
        for (size_t i = 0; i < (size_t)highPassRepeat; i++)
        {
            highPassFilters[i].processSamples(audioSamples + (unsigned long)bufferToFill.startSample,
                                              bufferToFill.numSamples);
        }

        for (size_t i = 0; i < (size_t)lowPassRepeat; i++)
        {
            lowPassFilters[i].processSamples(audioSamples + (unsigned long)bufferToFill.startSample,
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

AudioFileBufferRef SamplePlayer::getBufferRef()
{
    return audioBufferRef;
}

float SamplePlayer::freqForFilterDbReduction(bool isHighPass, float filterFreq, float dbReductionRequired,
                                             int filterRepeat)
{
    // how many decibels per octave we loose per filter (since they are second order this is const)
    float defaultFilterSlope = 12.0;
    // the total slope in db/octave of this set of filters
    float slope = defaultFilterSlope * filterRepeat;

    // how many octave change to achieve this gain reduction
    float octaveDistance = dbReductionRequired / slope;

    // the multiplier to apply the octave distance to the base frequency
    float multiplier = std::pow(2.0f, octaveDistance);

    float response = 0;

    if (isHighPass)
    {
        response = filterFreq / multiplier;
    }
    else
    {
        response = filterFreq * multiplier;
    }

    if (response < 0.0)
    {
        response = 0;
    }

    if (response > AUDIO_FRAMERATE / 2.0f)
    {
        response = AUDIO_FRAMERATE / 2.0f;
    }

    return response;
}

int SamplePlayer::getHighPassRepeat()
{
    return highPassRepeat;
}

void SamplePlayer::setHighPassRepeat(int repeat)
{
    if (repeat < 0 || repeat > SAMPLEPLAYER_MAX_FILTER_REPEAT)
    {
        return;
    }
    highPassRepeat = repeat;
}

int SamplePlayer::getLowPassRepeat()
{
    return lowPassRepeat;
}

void SamplePlayer::setLowPassRepeat(int v)
{
    if (v < 0 || v > SAMPLEPLAYER_MAX_FILTER_REPEAT)
    {
        return;
    }
    lowPassRepeat = v;
}