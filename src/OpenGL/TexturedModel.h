#ifndef DEF_TEXTURED_MODEL
#define DEF_TEXTURED_MODEL

#include "GraphicModel.h"
#include "TextureManager.h"
#include <memory>

class TexturedModel : public GraphicModel
{
  public:
    virtual ~TexturedModel();
    virtual void registerGlObjects() override;
    virtual void drawGlObjects() override;
    virtual void disable() override;

    /**
     Set disabled to false if disable
     was called before. This requires
     calling registerGlObjects to reallocate
     the texture and other gl objects
     before displaying anything.
     */
    virtual void reenable() override;

  protected:
    int textureWidth;
    int textureHeight;
    std::shared_ptr<std::vector<float>> texture;
    std::vector<unsigned char> textureBytes;
    // texture buffer object identifier
    GLuint tbo;

    // reference to the sample we want to display
    std::shared_ptr<SamplePlayer> displayedSample;

    bool reuseTexture;

    // a shared reference to an object that helps reusing textures so that duplicates
    // or same-file reimport share the same texture.
    juce::SharedResourcePointer<TextureManager> textureManager;
};

#endif