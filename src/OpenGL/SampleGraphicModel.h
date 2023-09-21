#ifndef DEF_SAMPLE_GRAPHIC_MODEL
#define DEF_SAMPLE_GRAPHIC_MODEL

#include <cstdint>
#include <vector>

#include "../Audio/SamplePlayer.h"
#include "TextureManager.h"
#include "TexturedModel.h"
#include "Vertex.h"
#include "juce_opengl/opengl/juce_gl.h"

enum VerticeProperty
{
    TEXTURE_POSITION_X,
    TEXTURE_POSITION_Y,
    VERTEX_POSITION_X,
    VERTEX_POSITION_Y,
    VERTEX_ALPHA_LEVEL
};

enum VerticeLineType
{
    HORIZONTAL_VERTEX_LINE,
    VERTICAL_VERTEX_LINE
};

class SampleGraphicModel : public TexturedModel
{
  public:
    /**
     * @brief      Constructs a new instance.
     */
    SampleGraphicModel(std::shared_ptr<SamplePlayer>, juce::Colour);

    /**
     * @brief      Initializes the position drag of this  object.
     */
    void initDrag();

    /**
     * @brief      Update position drag of this object.
     *
     * @param[in]  frameMove  How many audio frame the sample must be moved from prev pos.
     */
    void updateDrag(int frameMove);

    /**
     * @brief      Finds the intensity of the texture at this click position.
     *
     * @param[in]  x     horizontal texture position ratio (between 0 and 1)
     * @param[in]  y     vertical texture position ratio (between 0 and 1)
     *
     * @return     texture intensity between 0 and 1
     */
    float textureIntensity(float x, float y);

    /**
     * @brief      Get the position of this sample in the global
     *             track in frames.
     *
     * @return     The frame position.
     */
    juce::int64 getFramePosition();

    /**
     * @brief      Get the sample length in frames.
     *
     * @return     The frame length.
     */
    juce::int64 getFrameLength();

    /**
     * @brief      Change the color of the sample
     */
    void setColor(juce::Colour &);

    /**
     * @brief      Gather values from the sample player, sets
     *             the properties in this object, and re-generate
     *             the vertice data.
     *
     * @param[in]  sp    SamplePlayer to draw using this object.
     */
    void reloadSampleData(std::shared_ptr<SamplePlayer> sp);

    /**
     * @brief      Gets the rectangle for the pixel bounds of the core sample
     *             section (This will no include the filters fade out part)..
     *
     * @param[in]  viewPosition  The view position
     * @param[in]  viewScale     The view scale
     * @param[in]  viewHeight    The view height
     *
     * @return     The pixel bounds.
     */
    std::vector<juce::Rectangle<float>> getPixelBounds(float viewPosition, float viewScale, float viewHeight);

  private:
    int getTextureIndex(int freqIndex, int timeIndex, int freqDuplicateShift, bool isLeftChannel);

    void generateAndUploadVerticesToGPU(float leftX, float rightX, float fadeInFrames, float fadeOutFrames);

    void connectSquareFromVertexIds(size_t, size_t, size_t, size_t);

    void uploadVerticesToGpu();
    int isFullyFilteredArea(float y);

    /**
     * Read the data from the stored fft, and load it into the texture array
     * in a format friendly to OpenGL textures.
     * @param ffts         The array holding the stored fft data loaded by SamplePlayer.
     * @param fftCount     How many fft this array contains.
     * @param channelCount How many channels this array contains.
     */
    void loadFftDataToTexture(std::shared_ptr<std::vector<float>> ffts);

    /**
     * @brief      Updates the filters gain reduction steps we store for visualization.
     *             Will find for each FILTERS_FADE_STEP_DB decibels increment the freq at which
     *             the filters are reducting volume to this intensity.
     *
     * @param      sp    The samplePlayer object that holds the filters.
     */
    void updateFiltersGainReductionSteps(std::shared_ptr<SamplePlayer> sp);

    /**
     * @brief      For each sample object vertice in the specified line, set its values.
     *
     * @param[in]  targetPropertyToSet  The target property to set
     * @param[in]  lineType             The line type
     * @param[in]  lineIndex            The line index
     * @param[in]  value                The value
     */
    void setVerticeLineValue(VerticeProperty targetPropertyToSet, VerticeLineType lineType, int lineIndex, float value);

    /**
     * @brief      Sets an individual vertex vertex property (position, color, texture position).
     *
     * @param      vertexToChange    The vertex to change
     * @param[in]  propertyToChange  The property to change
     * @param[in]  value             The value
     */
    void setVertexProperty(Vertex &vertexToChange, VerticeProperty propertyToChange, float value);

    /**
     * @brief      Return a reference to the upper left vertex (without filters fade out parts)
     *
     * @return     The upper left corner.
     */
    Vertex &getUpperLeftCorner();

    /**
     * @brief      Return a reference to the upper right vertex (without filters fade out parts)
     *
     * @return     The upper right corner.
     */
    Vertex &getUpperRightCorner();

    /**
     * @brief      Gets the filtering level. It's 0 if the area is unfiltered, and goes
     *             from 1 to FILTERS_FADE_DEFINITION for the filtered area. If equal to
     *              FILTERS_FADE_DEFINITION+1, it means that it's totally filtered.
     *
     * @param[in]  yPosition  The y position between 0 and 1 from top to bottom.
     *
     * @return     The filtering level between 0 and  FILTERS_FADE_DEFINITION+1.
     */
    int getFilteringLevel(float yPosition);

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

    // number of vertices lines in the mesh grid
    int noVerticalVerticeLines, noHorizontalVerticeLines;

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