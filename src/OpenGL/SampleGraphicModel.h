#ifndef DEF_SAMPLE_GRAPHIC_MODEL
#define DEF_SAMPLE_GRAPHIC_MODEL

#include <cstdint>
#include <vector>

#include "../Audio/SamplePlayer.h"
#include "TextureManager.h"
#include "TexturedModel.h"
#include "Vertex.h"
#include "juce_opengl/opengl/juce_gl.h"

class SampleGraphicModel : public TexturedModel
{
  public:
    SampleGraphicModel(std::shared_ptr<SamplePlayer>, juce::Colour);
    void initDrag();
    void updateDrag(int);
    float textureIntensity(float x, float y);
    juce::int64 getFramePosition();
    juce::int64 getFrameLength();
    void setColor(juce::Colour &);
    void reloadSampleData(std::shared_ptr<SamplePlayer> sp);
    std::vector<juce::Rectangle<float>> getPixelBounds(float viewPosition, float viewScale, float viewHeight);

  private:
    int getTextureIndex(int freqIndex, int timeIndex, int freqDuplicateShift, bool isLeftChannel);

    void generateAndUploadVerticesToGPU(float leftX, float rightX, float lowFreq, float highFreq, float fadeInFrames,
                                        float fadeOutFrames);

    void connectSquareFromVertexIds(size_t, size_t, size_t, size_t);

    void uploadVerticesToGpu();
    int isFilteredArea(float y);

    /**
     * Read the data from the stored fft, and load it into the texture array
     * in a format friendly to OpenGL textures.
     * @param ffts         The array holding the stored fft data loaded by SamplePlayer.
     * @param fftCount     How many fft this array contains.
     * @param channelCount How many channels this array contains.
     */
    void loadFftDataToTexture(std::shared_ptr<std::vector<float>> ffts, int fftCount, int channelCount);

    /**
     * @brief      Updates the filters gain reduction steps we store for visualization.
     *             Will find for each FILTERS_FADE_STEP_DB decibels increment the freq at which
     *             the filters are reducting volume to this intensity.
     *
     * @param      sp    The samplePlayer object that holds the filters.
     */
    void updateFiltersGainReductionSteps(std::shared_ptr<SamplePlayer> sp);

    ///////////////////////////////

    // the FILTERS_FADE_DEFINITION frequencies for which the volume is reduced
    // by the low pass filter for each FILTERS_FADE_STEP_DB decibels per step.
    std::vector<float> lowPassGainReductionSteps;

    // the FILTERS_FADE_DEFINITION frequencies for which the volume is reduced
    // by the high pass filter for each FILTERS_FADE_STEP_DB decibels per step.
    std::vector<float> highPassGainReductionSteps;

    int dragStartPosition;
    int lastWidth;

    int numFfts;
    int numChannels;
    int channelTextureShift;
    juce::Colour color;
    float lastLowPassFreq, lastHighPassFreq;
    int horizontalScaleMultiplier;
    float lastFadeInFrameLength, lastFadeOutFrameLength;

    // NOTE: the sample can be moved inside their audio buffer so their texture
    // position can change. On top of that, there is a fade in and fade out
    // length that we will reflect on displayed samples by fading colors to transparent
    // at the start and end of each samples proportional to fade length.

    // position between 0 and 1 of samplePlayer startPosition and endPosition relative to audio buffer
    float bufferStartPosRatio, bufferEndPosRatio;

    // audio buffer position between 0 and 1 start and end position where it's not ongoing fade in or fade out gain
    // ramps
    float bufferStartPosRatioAfterFadeIn, bufferEndPosRatioBeforeFadeOut;
};

#endif // DEF_SAMPLE_GRAPHIC_MODEL