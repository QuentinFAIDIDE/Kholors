#ifndef DEF_UNIT_CONVERTER_HPP
#define DEF_UNIT_CONVERTER_HPP

#include "../Config.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>

class UnitConverter
{
  public:
    static float gainToDb(float val);
    static float gainToDbInv(float val);

    /**
     * Projects the [0, FREQVIEW_SAMPLE_FFT_SCOPE_SIZE] index of the stored fft data
     * into the [0, FREQVIEW_SAMPLE_FFT_SIZE/2] range of the Hz frequencies bins
     * that are result of the FFT.
     * Ie use this to convert an index of the stored fft data into an index of
     * the fft computing result.
     * @param  k index in range [0, FREQVIEW_SAMPLE_FFT_SCOPE_SIZE] (powered of .88 log10 relation to Hz)
     * @return   index in range [0, FREQVIEW_SAMPLE_FFT_SIZE/2] (linear relation to Hz)
     */
    static int magnifyFftIndex(int k);
    /**
     * Exact inverse of magnifyFftIndex. Will take an index from the fft and convert it
     * back into an index of the stored data.
     * @param  k index in range [0, FREQVIEW_SAMPLE_FFT_SIZE/2] (linear relation to Hz)
     * @return   index in range [0, FREQVIEW_SAMPLE_FFT_SCOPE_SIZE] (powered of .88 log10 relation to Hz)
     */
    static int magnifyFftIndexInv(int k);

    /**
     * Subsequent layer of magnification to write texture data
     * to opengl for individual samples. Sort of zoom in the index.
     * @param  k index to magnify.
     * @return   index after zooming in.
     */
    static int magnifyTextureFrequencyIndex(int k);
    /**
     * Invert of the magnifyTextureFrequency function.
     */
    static int magnifyTextureFrequencyIndexInv(int k);

    /**
     * Increase contrast from stored ffts to displayed
     * texture. Maps from the range [MIN_DB, MAX_DB] to
     * [0, 1]
     */
    static float magnifyIntensity(float dbValue);

    /**
     * polynomial transformation to zoom in the middle of frequencies
     * and make important frequencies stand out.
     * Works on floats between 0 and 1.
     */
    static float polylens(float v);
    /**
     * Invert of polylens function.
     */
    static float polylensInv(float v);

    /**
     * Sigmoid activation function to try to increase contrast in fft intensities.
     * Works on floats between 0 and 1.
     */
    static float sigmoid(float val);
    /**
     * Sigmoid activation function to try to increase contrast in fft intensities.
     */
    static float sigmoidInv(float val);

  private:
    static float magnifyFftPrecomputedFactor1;
    static float magnifyFftPrecomputedFactor2;

    static float magnifyFftInvPrecomputedFator1;
    static float magnifyFftInvPrecomputedFator2;
    static float magnifyFftInvPrecomputedFator3;
};

#endif // DEF_UNIT_CONVERTER_HPP