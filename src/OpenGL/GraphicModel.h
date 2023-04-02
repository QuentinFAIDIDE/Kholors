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

 protected:
  // polynomial transformation to zoom in the middle of frequencies and make
  // things easier to view. Works on floats between 0 and 1.
  float polylens(float v) {
    if (v < 0.5) {
      return std::pow(v, 0.3f) * (0.5 / (std::pow(0.5, 0.3)));
    } else {
      return 0.5 + std::pow(v - 0.5, 2.0f) * (0.5 / (std::pow(0.5, 2.0f)));
    }
  }

  // Sigmoid activation function to try to increase contrast in fft intensities.
  // Works on floats between 0 and 1.
  float sigmoid(float val) { return 1 / (1 + exp(-val)); };

  std::vector<Vertex> _vertices;
  std::vector<unsigned int> _triangleIds;
  // vertex buffer object identifier
  GLuint _vbo;
  // index buffer object identifier (ids of vertices for triangles to draw)
  GLuint _ebo;
  // vertex array object to draw with a oneliner
  GLuint _vao;

  bool _loaded;
  bool _disabled;
};

#endif  // DEF_GRAPHIC_MODEL