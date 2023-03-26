#ifndef DEF_SAMPLE_GRAPHIC_MODEL
#define DEF_SAMPLE_GRAPHIC_MODEL

#include <vector>

#include "../Audio/SamplePlayer.h"
#include "Vertex.h"
#include "juce_opengl/opengl/juce_gl.h"

class SampleGraphicModel {
 public:
  SampleGraphicModel(SamplePlayer*);
  ~SampleGraphicModel();
  void registerGlObjects();
  void drawGlObjects();
  void disable();

 private:
  std::vector<Vertex> _vertices;
  std::vector<unsigned int> _triangleIds;
  int _textureWidth;
  int _textureHeight;
  std::vector<float> _texture;
  std::vector<unsigned char> _textureBytes;
  // vertex buffer object identifier
  GLuint _vbo;
  // index buffer object identifier (ids of vertices for triangles to draw)
  GLuint _ebo;
  // texture buffer object identifier
  GLuint _tbo;
  // vertex array object to draw with a oneliner
  GLuint _vao;

  bool _loaded;
  bool _disabled;

  // Sigmoid activation function to try to increase contrast in fft intensities
  float sigmoid(float);
};

#endif  // DEF_SAMPLE_GRAPHIC_MODEL