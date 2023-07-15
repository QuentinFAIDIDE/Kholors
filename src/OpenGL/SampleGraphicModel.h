#ifndef DEF_SAMPLE_GRAPHIC_MODEL
#define DEF_SAMPLE_GRAPHIC_MODEL

#include <cstdint>
#include <vector>

#include "../Audio/SamplePlayer.h"
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
    void loadVerticeData(std::shared_ptr<SamplePlayer> sp);
    std::vector<juce::Rectangle<float>> getPixelBounds(float viewPosition, float viewScale, float viewHeight);

  private:
    int dragStartPosition;
    int lastWidth;
    int getTextureIndex(int freqIndex, int timeIndex, int freqDuplicateShift, bool isLeftChannel);

    void generateAndUploadVerticesToGPU(float leftX, float rightX, float lowFreq, float highFreq);

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

    int numFfts;
    int numChannels;
    int channelTextureShift;
    juce::Colour color;
    float lastLowPassFreq, lastHighPassFreq;
    int horizontalScaleMultiplier;
    // the position between 0 and 1 of samplePlayer startPosition and endPosition relative to audio buffer
    float startPositionNormalized, endPositionNormalised;
};

#endif // DEF_SAMPLE_GRAPHIC_MODEL