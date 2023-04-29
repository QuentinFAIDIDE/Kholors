#ifndef DEF_GRAPHIC_MODEL
#define DEF_GRAPHIC_MODEL

#include <vector>

#include "../Audio/SamplePlayer.h"
#include "Vertex.h"
#include "juce_opengl/opengl/juce_gl.h"

class GraphicModel
{
  public:
    virtual void registerGlObjects() = 0;
    virtual void drawGlObjects() = 0;
    virtual void disable() = 0;
    virtual bool isDisabled()
    {
        return disabled;
    };

  protected:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> triangleIds;
    // vertex buffer object identifier
    GLuint vbo;
    // index buffer object identifier (ids of vertices for triangles to draw)
    GLuint ebo;
    // vertex array object to draw with a oneliner
    GLuint vao;

    bool loaded;
    bool disabled;

    juce::CriticalSection loadingMutex;
};

#endif // DEF_GRAPHIC_MODEL