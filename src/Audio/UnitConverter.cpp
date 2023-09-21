#include "UnitConverter.h"
#include <cmath>

float UnitConverter::magnifyFftIndex(float k)
{
    // the transfo is mainly made of three steps:
    // log10 normalized to [0, 1] using A and B
    // then set to power of C to zoom in a little
    // then normalized to fit in [0, FREQVIEW_SAMPLE_FFT_SIZE]

    // this has been simplfied and refactored to fit in a single
    // formula and precompute as much as possible

    // Disabled during FFT engine migration.
    // TODO: recompute factors

    return juce::jlimit(0.0f, float(FFT_OUTPUT_NO_FREQS), k / float(FFT_STORAGE_SCOPE_SIZE));
}

float UnitConverter::magnifyFftIndexInv(float k)
{
    return juce::jlimit(0.0f, float(FFT_STORAGE_SCOPE_SIZE), k / float(FFT_OUTPUT_NO_FREQS));
}

float UnitConverter::magnifyTextureFrequencyIndex(float k)
{
    // TODO: develop and factor the mixing of polylens to try
    // to find factors to precompute

    // we apply our polynomial lens transformation to zoom in a bit
    float position = k / float(FFT_STORAGE_SCOPE_SIZE - 1);
    float index = polylens(position) * float(FFT_STORAGE_SCOPE_SIZE - 1);
    return juce::jlimit(0.0f, float(FFT_STORAGE_SCOPE_SIZE - 1), index);
}

float UnitConverter::magnifyTextureFrequencyIndexInv(float k)
{
    float position = float(k) / float(FFT_STORAGE_SCOPE_SIZE - 1);
    float index = polylensInv(position) * float(FFT_STORAGE_SCOPE_SIZE - 1);
    return juce::jlimit(0.0f, float(FFT_STORAGE_SCOPE_SIZE - 1), index);
}

float UnitConverter::sigmoid(float val)
{
    return 1 / (1 + exp(-val));
}

float UnitConverter::sigmoidInv(float val)
{
    return -std::log((1.0 / val) - 1.0);
}

float UnitConverter::polylens(float v)
{
    return 1.0f - zoomInRangeInv(1.0f - v);
}

float UnitConverter::polylensInv(float v)
{
    return 1.0f - zoomInRange(1.0f - v);
}

float UnitConverter::magnifyIntensity(float input)
{
    // TODO: factor and develop calculus to save on computing power.
    float intensity = input;
    // we need to normalize the frequency by mapping the range
    intensity = juce::jmap(intensity, MIN_DB, MAX_DB, 0.0f, 1.0f);
    return intensity;
}

float UnitConverter::verticalPositionToFrequency(int y, int viewHeight)
{
    // REMINDER: upper half (below half height) is the first
    // fft with lower frequencies below. second halve freqs are the opposite disposition.

    // freqRatio is the ratio from 0 to max frequency (AUDIO_FRAMRATE/2).
    // It's not linear to freqs, we therefore need to invert our index correction
    // from texture freq index to storage freq index and then from storage
    // freq index to fft index.
    float freqRatio = 0.0f;
    if (y < (viewHeight >> 1))
    {
        freqRatio = 1.0f - (float(y) / float(viewHeight >> 1));
    }
    else
    {
        freqRatio = (float(y) / float(viewHeight >> 1)) - 1.0f;
    }

    // texture are flipped
    freqRatio = 1.0 - freqRatio;

    // apply back the index transformation to make it linear to frequencies
    float textureFreqIndex = freqRatio * float(FFT_STORAGE_SCOPE_SIZE - 1);
    float storageFreqIndex =
        FFT_STORAGE_SCOPE_SIZE - (UnitConverter::magnifyTextureFrequencyIndex(textureFreqIndex) + 1);
    float fftFreqIndex = UnitConverter::magnifyFftIndex(storageFreqIndex);
    // map the fft index to a frequency
    float freq = fftFreqIndex * (float(AUDIO_FRAMERATE) / float(2 * FFT_OUTPUT_NO_FREQS));

    return juce::jlimit(0.0f, float(AUDIO_FRAMERATE >> 1), freq);
}

float UnitConverter::freqToPositionRatio(float freq)
{
    float fftIndex = freq * (float(FFT_OUTPUT_NO_FREQS) / float(AUDIO_FRAMERATE >> 1));
    float storageIndex = UnitConverter::magnifyFftIndexInv(fftIndex);
    storageIndex = float(FFT_STORAGE_SCOPE_SIZE) - storageIndex;
    float textureIndex = UnitConverter::magnifyTextureFrequencyIndexInv(storageIndex);
    float ratio = (float(textureIndex) / float(FFT_STORAGE_SCOPE_SIZE));
    return juce::jlimit(0.0f, 1.0f, 1.0f - ratio);
}

float UnitConverter::zoomInRange(float v)
{
    if (v < 0.4f)
    {
        return 0.3f * v;
    }
    else
    {
        return 0.12f + ((v - 0.4f) * 1.4666666666666668f);
    }
}

float UnitConverter::zoomInRangeInv(float v)
{
    if (v < 0.12f)
    {
        return v * 3.3333333333333335f;
    }
    else
    {
        return 0.4f + ((v - 0.12f) * 0.6818181818181818f);
    }
}

float UnitConverter::dbFromBufferChannel(const juce::AudioSourceChannelInfo &buffer, int chan)
{
    auto rawValue =
        buffer.buffer->getMagnitude(chan % buffer.buffer->getNumChannels(), buffer.startSample, buffer.numSamples);
    return juce::Decibels::gainToDecibels(rawValue);
}
