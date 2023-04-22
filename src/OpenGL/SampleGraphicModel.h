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
    SampleGraphicModel(SamplePlayer *, juce::Colour);
    void initDrag();
    void updateDrag(int);
    float textureIntensity(float x, float y);
    juce::int64 getFramePosition();
    juce::int64 getFrameLength();
    void setColor(juce::Colour &);
    void updatePropertiesAndUploadToGpu(SamplePlayer *sp);

  private:
    void transformIntensity(float &);
    int transformFrequencyLocation(int);
    int dragStartPosition;
    int lastWidth;
    int getTextureIndex(int freqIndex, int timeIndex, int freqDuplicateShift, bool isLeftChannel);

    void generateAndUploadVertices(float leftX, float rightX);

    void connectSquareFromVertexIds(size_t, size_t, size_t, size_t);

    int horizontalScaleMultiplier;

    void uploadVerticesToGpu();

    int numFfts;
    int numChannels;
    int channelTextureShift;
    juce::Colour color;

    // the position between 0 and 1 of samplePlayer startPosition and endPosition relative to audio buffer
    float startPositionNormalized, endPositionNormalised;
};

#endif // DEF_SAMPLE_GRAPHIC_MODEL