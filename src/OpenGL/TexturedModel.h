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
  int _textureWidth;
  int _textureHeight;
  std::vector<float> _texture;
  std::vector<unsigned char> _textureBytes;
  // texture buffer object identifier
  GLuint _tbo;
};

#endif