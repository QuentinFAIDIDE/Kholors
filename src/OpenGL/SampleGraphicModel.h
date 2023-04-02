#ifndef DEF_SAMPLE_GRAPHIC_MODEL
#define DEF_SAMPLE_GRAPHIC_MODEL

#include <vector>

#include "../Audio/SamplePlayer.h"
#include "TexturedModel.h"
#include "Vertex.h"
#include "juce_opengl/opengl/juce_gl.h"

class SampleGraphicModel : public TexturedModel {
 public:
  SampleGraphicModel(SamplePlayer*);
};

#endif  // DEF_SAMPLE_GRAPHIC_MODEL