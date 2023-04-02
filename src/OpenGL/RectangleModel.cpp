#include "RectangleModel.h"

#include "juce_opengl/opengl/juce_gl.h"

using namespace juce::gl;

// registerGlObjects registers vertices, triangle indices and textures.
// Warning, call it from the openGL thread using the juce opengl context
// utility for that !
void RectangleModel::registerGlObjects() {
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
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  // color
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                        (void *)(4 * sizeof(float)));
  glEnableVertexAttribArray(1);

  _loaded = true;
}

// drawGlObjects will draw the sample. Watch out, it needs to be called
// from the openGL drawing function of juce OpenGL context. Make sure
// to use/bind the right shader before calling this.
void RectangleModel::drawGlObjects() {
  // abort if openGL object are not loaded
  if (!_loaded || _disabled) {
    return;
  }

  glBindVertexArray(_vao);
  glDrawElements(GL_TRIANGLES, _triangleIds.size(), GL_UNSIGNED_INT, 0);

  glBindVertexArray(0);
}

void RectangleModel::disable() {
  if (!_disabled && _loaded) {
    _disabled = true;
    glDeleteVertexArrays(1, &_vao);
    glDeleteBuffers(1, &_vbo);
    glDeleteBuffers(1, &_ebo);
  }
}

RectangleModel::~RectangleModel() {
  // free resources if sample model is loaded and enabled
  if (_loaded && !_disabled) {
    glDeleteVertexArrays(1, &_vao);
    glDeleteBuffers(1, &_vbo);
    glDeleteBuffers(1, &_ebo);
  }
}