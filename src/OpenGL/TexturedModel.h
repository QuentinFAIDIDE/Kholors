#ifndef DEF_TEXTURED_MODEL
#define DEF_TEXTURED_MODEL

#include "GraphicModel.h"

class TexturedModel : public GraphicModel {
 public:
  virtual ~TexturedModel();
  virtual void registerGlObjects() override;
  virtual void drawGlObjects() override;
  virtual void disable() override;

 protected:
  // Sigmoid activation function to try to increase contrast in fft intensities
  float sigmoid(float val) { return 1 / (1 + exp(-val)); };

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
};

#endif