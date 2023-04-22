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
    void move(int64_t position);
    void initDrag();
    void updateDrag(int);
    float textureIntensity(float x, float y);
    juce::int64 getFramePosition();
    juce::int64 getFrameLength();
    void setColor(juce::Colour &);
    void updateProperties(SamplePlayer *sp);

  private:
    void transformIntensity(float &);
    int transformFrequencyLocation(int);
    int dragStartPosition;
    int lastWidth;
    int getTextureIndex(int freqIndex, int timeIndex, int freqDuplicateShift, bool isLeftChannel);

    int horizontalScaleMultiplier;

    void uploadVerticesToGpu();

    int numFfts;
    int numChannels;
    int channelTextureShift;
    juce::Colour color;
};

#endif // DEF_SAMPLE_GRAPHIC_MODEL