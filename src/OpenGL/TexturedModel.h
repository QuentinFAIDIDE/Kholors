#ifndef DEF_TEXTURED_MODEL
#define DEF_TEXTURED_MODEL

#include "GraphicModel.h"

class TexturedModel : public GraphicModel
{
  public:
    virtual ~TexturedModel();
    virtual void registerGlObjects() override;
    virtual void drawGlObjects() override;
    virtual void disable() override;

  protected:
    int textureWidth;
    int textureHeight;
    std::vector<float> texture;
    std::vector<unsigned char> textureBytes;
    // texture buffer object identifier
    GLuint tbo;
};

#endif