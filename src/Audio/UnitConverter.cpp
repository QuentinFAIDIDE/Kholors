#include "UnitConverter.h"
#include <cmath>

float UnitConverter::magnifyFftPrecomputedFactor1 =
    ((FFT_MAGNIFY_B * FFT_MAGNIFY_C) / float(FREQVIEW_SAMPLE_FFT_SCOPE_SIZE));
float UnitConverter::magnifyFftPrecomputedFactor2 = 0.5 * std::pow(FFT_MAGNIFY_A, FFT_MAGNIFY_C);

float UnitConverter::magnifyFftInvPrecomputedFator1 = FREQVIEW_SAMPLE_FFT_SCOPE_SIZE / (FFT_MAGNIFY_B * FFT_MAGNIFY_C);
float UnitConverter::magnifyFftInvPrecomputedFator2 =
    magnifyFftInvPrecomputedFator1 *
    log10(2 / (std::pow(FFT_MAGNIFY_A, FFT_MAGNIFY_C) * (FREQVIEW_SAMPLE_FFT_SIZE >> 1)));

float UnitConverter::fftToDb(float val)
{
    return juce::jlimit(MIN_DB, MAX_DB, juce::Decibels::gainToDecibels(val / (float)FREQVIEW_SAMPLE_FFT_SIZE));
};

float UnitConverter::fftToDbInv(float val)
{
    return juce::jlimit(0.0f, (float)FREQVIEW_SAMPLE_FFT_SIZE,
                        juce::Decibels::decibelsToGain(val) * (float)FREQVIEW_SAMPLE_FFT_SIZE);
}

float UnitConverter::magnifyFftIndex(float k)
{
    // the transfo is mainly made of three steps:
    // log10 normalized to [0, 1] using A and B
    // then set to power of C to zoom in a little
    // then normalized to fit in [0, FREQVIEW_SAMPLE_FFT_SIZE/2]

    // this has been simplfied and refactored to fit in a single
    // formula and precompute as much as possible

    float t1 = k * magnifyFftPrecomputedFactor1;
    float res = magnifyFftPrecomputedFactor2 * std::pow(10.0f, t1) * (FREQVIEW_SAMPLE_FFT_SIZE >> 1);
    return juce::jlimit(0.0f, float(FREQVIEW_SAMPLE_FFT_SIZE >> 1), res);
}

float UnitConverter::magnifyFftIndexInv(float k)
{
    float res = (magnifyFftInvPrecomputedFator1 * log10(k)) + magnifyFftInvPrecomputedFator2;
    return juce::jlimit(0.0f, float(FREQVIEW_SAMPLE_FFT_SCOPE_SIZE), res);
}

float UnitConverter::magnifyTextureFrequencyIndex(float k)
{
    // TODO: develop and factor the mixing of polylens to try
    // to find factors to precompute

    // we apply our polynomial lens transformation to zoom in a bit
    float position = k / float(FREQVIEW_SAMPLE_FFT_SCOPE_SIZE - 1);
    float index = polylens(position) * float(FREQVIEW_SAMPLE_FFT_SCOPE_SIZE - 1);
    return juce::jlimit(0.0f, float(FREQVIEW_SAMPLE_FFT_SCOPE_SIZE - 1), index);
}

float UnitConverter::magnifyTextureFrequencyIndexInv(float k)
{
    float position = float(k) / float(FREQVIEW_SAMPLE_FFT_SCOPE_SIZE - 1);
    float index = polylensInv(position) * float(FREQVIEW_SAMPLE_FFT_SCOPE_SIZE - 1);
    return juce::jlimit(0.0f, float(FREQVIEW_SAMPLE_FFT_SCOPE_SIZE - 1), index);
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
    return std::pow(v, 2.0f);
}

float UnitConverter::polylensInv(float v)
{
    return std::pow(v, 0.5f);
}

float UnitConverter::magnifyIntensity(float input)
{
    // TODO: factor and develop calculus to save on computing power.
    float intensity = input;
    // we need to normalize the frequency by mapping the range
    intensity = juce::jmap(intensity, MIN_DB, MAX_DB, 0.0f, 1.0f);
    // then we make it a little prettier with a sigmoid function
    // (increase contrasts)
    intensity = sigmoid((intensity * 12.0) - 6.0);
    // and finally we make sure it falls in range
    intensity = juce::jlimit(0.0f, 1.0f, intensity);
    return intensity;
}