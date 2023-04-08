#include "BackgroundModel.h"

#include "juce_opengl/opengl/juce_gl.h"

using namespace juce::gl;

BackgroundModel::BackgroundModel() {
  // init vertices
  _loaded = false;
  _disabled = false;

  _vertices.reserve(4);

  // TODO: move that into the config
  juce::Colour col(10, 10, 10);

  // NOTE: we use a unified vertex format that includes
  // texture coordinates because we can affort to send
  // it for the background even if we don't really use it

  // upper left corner 0
  _vertices.push_back(
      {{-1.0f, -1.0f},
       {col.getFloatRed(), col.getFloatGreen(), col.getFloatBlue(), 0.0f},
       {0.0f, 1.0f}});

  // upper right corner 1
  _vertices.push_back(
      {{1.0f, -1.0f},
       {col.getFloatRed(), col.getFloatGreen(), col.getFloatBlue(), 1.0f},
       {1.0f, 1.0f}});

  // lower right corner 2
  _vertices.push_back(
      {{1.0f, 1.0f},
       {col.getFloatRed(), col.getFloatGreen(), col.getFloatBlue(), 1.0f},
       {1.0f, 0.0f}});

  // lower left corner 3
  _vertices.push_back(
      {{-1.0f, 1.0f},
       {col.getFloatRed(), col.getFloatGreen(), col.getFloatBlue(), 1.0f},
       {0.0f, 0.0f}});

  // lower left triangle
  _triangleIds.push_back(0);
  _triangleIds.push_back(2);
  _triangleIds.push_back(3);

  // upper right triangle
  _triangleIds.push_back(0);
  _triangleIds.push_back(1);
  _triangleIds.push_back(2);
}

// registerGlObjects registers vertices, triangle indices and textures.
// Warning, call it from the openGL thread using the juce opengl context
// utility for that !
void BackgroundModel::registerGlObjects() {
  // generate objects
  glGenVertexArrays(1, &_vao);
  glGenBuffers(1, &_vbo);
  glGenBuffers(1, &_ebo);

  glBindVertexArray(_vao);

  // register and upload the vertices data
  glBindBuffer(GL_ARRAY_BUFFER, _vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * _vertices.size(),
               _vertices.data(), GL_STATIC_DRAW);
  // register and upload indices of the vertices to form the triangles
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               sizeof(unsigned int) * _triangleIds.size(), _triangleIds.data(),
               GL_STATIC_DRAW);

  // register the vertex attribute format

  // position
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  // color
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                        (void *)(2 * sizeof(float)));
  glEnableVertexAttribArray(1);
  // texture
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                        (void *)(6 * sizeof(float)));
  glEnableVertexAttribArray(2);

  _loaded = true;
}

// drawGlObjects will draw the sample. Watch out, it needs to be called
// from the openGL drawing function of juce OpenGL context. Make sure
// to use/bind the right shader before calling this.
void BackgroundModel::drawGlObjects() {
  // abort if openGL object are not loaded
  if (!_loaded || _disabled) {
    return;
  }

  glBindVertexArray(_vao);
  glDrawElements(GL_TRIANGLES, _triangleIds.size(), GL_UNSIGNED_INT, 0);

  glBindVertexArray(0);
}

void BackgroundModel::disable() {
  if (!_disabled && _loaded) {
    _disabled = true;
    glDeleteVertexArrays(1, &_vao);
    glDeleteBuffers(1, &_vbo);
    glDeleteBuffers(1, &_ebo);
  }
}

BackgroundModel::~BackgroundModel() {
  // free resources if sample model is loaded and enabled
  if (_loaded && !_disabled) {
    glDeleteVertexArrays(1, &_vao);
    glDeleteBuffers(1, &_vbo);
    glDeleteBuffers(1, &_ebo);
  }
}