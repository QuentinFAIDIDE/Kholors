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

/**
 * Projects the [0, FREQVIEW_SAMPLE_FFT_SCOPE_SIZE] index of the stored fft data
 * into the [0, FREQVIEW_SAMPLE_FFT_SIZE/2] range of the Hz frequencies bins
 * that are result of the FFT.
 * Ie use this to convert an index of the stored fft data into an index of
 * the fft computing result.
 * @param  k index in range [0, FREQVIEW_SAMPLE_FFT_SCOPE_SIZE] (powered of .88 log10 relation to Hz)
 * @return   index in range [0, FREQVIEW_SAMPLE_FFT_SIZE/2] (linear relation to Hz)
 */
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

/**
 * Exact inverse of magnifyFftIndex. Will take an index from the fft and convert it
 * back into an index of the stored data.
 * @param  k index in range [0, FREQVIEW_SAMPLE_FFT_SIZE/2] (linear relation to Hz)
 * @return   index in range [0, FREQVIEW_SAMPLE_FFT_SCOPE_SIZE] (powered of .88 log10 relation to Hz)
 */
int UnitConverter::magnifyFftIndexInv(int k)
{
    float res = (magnifyFftInvPrecomputedFator1 * log10(k)) + magnifyFftInvPrecomputedFator2;
    return juce::jlimit(0, FREQVIEW_SAMPLE_FFT_SCOPE_SIZE, int(res + 0.5f));
}