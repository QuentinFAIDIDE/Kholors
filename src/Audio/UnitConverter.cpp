#include "UnitConverter.h"

float UnitConverter::magnifyFftPrecomputedFactor1 =
    ((FFT_MAGNIFY_B * FFT_MAGNIFY_C) / float(FREQVIEW_SAMPLE_FFT_SCOPE_SIZE));
float UnitConverter::magnifyFftPrecomputedFactor2 = 0.5 * std::pow(FFT_MAGNIFY_A, FFT_MAGNIFY_C);

float UnitConverter::magnifyFftInvPrecomputedFator1 = FREQVIEW_SAMPLE_FFT_SCOPE_SIZE / (FFT_MAGNIFY_B * FFT_MAGNIFY_C);
float UnitConverter::magnifyFftInvPrecomputedFator2 =
    magnifyFftInvPrecomputedFator1 * log10(2 / (std::pow(FFT_MAGNIFY_A, FFT_MAGNIFY_C) * FREQVIEW_SAMPLE_FFT_SIZE));

float UnitConverter::gainToDb(float val)
{
    return juce::jlimit(MIN_DB, MAX_DB,
                        juce::Decibels::gainToDecibels(val) -
                            juce::Decibels::gainToDecibels((float)FREQVIEW_SAMPLE_FFT_SIZE));
};

float UnitConverter::gainToDbInv(float val)
{
    return juce::jlimit(0.0f, 1.0f, val + juce::Decibels::gainToDecibels((float)FREQVIEW_SAMPLE_FFT_SIZE));
}

int UnitConverter::magnifyFftIndex(int k)
{
    // the transfo is mainly made of three steps:
    // log10 normalized to [0, 1] using A and B
    // then set to power of C to zoom in a little
    // then normalized to fit in [0, FREQVIEW_SAMPLE_FFT_SIZE/2]

    // this has been simplfied and refactored to fit in a single
    // formula and precompute as much as possible

    float t1 = k * magnifyFftPrecomputedFactor1;
    float res = magnifyFftPrecomputedFactor2 * std::pow(10.0f, t1) * FREQVIEW_SAMPLE_FFT_SIZE;
    return juce::jlimit(0, FREQVIEW_SAMPLE_FFT_SIZE >> 1, int(res + 0.5f));
}

int UnitConverter::magnifyFftIndexInv(int k)
{
    float res = (magnifyFftInvPrecomputedFator1 * log10(k)) + magnifyFftInvPrecomputedFator2;
    return juce::jlimit(0, FREQVIEW_SAMPLE_FFT_SCOPE_SIZE, int(res + 0.5f));
}

int UnitConverter::magnifyTextureFrequencyIndex(int k)
{
    // TODO: develop and factor the mixing of polylens to try
    // to find factors to precompute

    // we apply our polynomial lens transformation to zoom in a bit
    int freqiZoomed = int(polylens(float(k) / float(FREQVIEW_SAMPLE_FFT_SCOPE_SIZE)) * FREQVIEW_SAMPLE_FFT_SCOPE_SIZE);
    // let's take extra care that it's inbound
    return juce::jlimit(0, FREQVIEW_SAMPLE_FFT_SCOPE_SIZE - 1, freqiZoomed);
}

int UnitConverter::magnifyTextureFrequencyIndexInv(int k)
{
    // TODO
    return 0;
}

float UnitConverter::sigmoid(float val)
{
    return 1 / (1 + exp(-val));
}

float UnitConverter::sigmoidInv(float val)
{
    // TODO
    return 0.0f;
}

float UnitConverter::polylens(float v)
{
    if (v < 0.5)
    {
        return std::pow(v, 0.3f) * (0.5 / (std::pow(0.5, 0.3)));
    }
    else
    {
        return 0.5 + std::pow(v - 0.5, 2.0f) * (0.5 / (std::pow(0.5, 2.0f)));
    }
}

float UnitConverter::polylensInv(float v)
{
    // TODO
    return 0.0f;
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