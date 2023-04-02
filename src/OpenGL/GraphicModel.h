#ifndef DEF_GRAPHIC_MODEL
#define DEF_GRAPHIC_MODEL

#include <vector>

#include "../Audio/SamplePlayer.h"
#include "Vertex.h"
#include "juce_opengl/opengl/juce_gl.h"

class GraphicModel {
 public:
  virtual void registerGlObjects() = 0;
  virtual void drawGlObjects() = 0;
  virtual void disable() = 0;
};

#endif  // DEF_GRAPHIC_MODEL