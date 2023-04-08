#include "TexturedModel.h"

#include "juce_opengl/opengl/juce_gl.h"

using namespace juce::gl;

// registerGlObjects registers vertices, triangle indices and textures.
// Warning, call it from the openGL thread using the juce opengl context
// utility for that !
void TexturedModel::registerGlObjects() {
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

  // register the texture
  glGenTextures(1, &_tbo);
  glBindTexture(GL_TEXTURE_2D, _tbo);
  // set the texture wrapping
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  // set the filtering parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  // send the texture to the gpu
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, _textureWidth, _textureHeight, 0,
               GL_RGBA, GL_FLOAT, _texture.data());
  glGenerateMipmap(GL_TEXTURE_2D);

  _loaded = true;
}

// drawGlObjects will draw the sample. Watch out, it needs to be called
// from the openGL drawing function of juce OpenGL context. Make sure
// to use/bind the right shader before calling this.
void TexturedModel::drawGlObjects() {
  // abort if openGL object are not loaded
  if (!_loaded || _disabled) {
    return;
  }

  glActiveTexture(GL_TEXTURE0);  // <- might only be necessary on some GPUs
  glBindTexture(GL_TEXTURE_2D, _tbo);
  glBindVertexArray(_vao);

  // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
  glDrawElements(GL_TRIANGLES, _triangleIds.size(), GL_UNSIGNED_INT, 0);

  glBindVertexArray(0);
  glBindTexture(GL_TEXTURE_2D, 0);
}

void TexturedModel::disable() {
  if (!_disabled && _loaded) {
    _disabled = true;
    glDeleteVertexArrays(1, &_vao);
    glDeleteBuffers(1, &_vbo);
    glDeleteBuffers(1, &_ebo);
    // glDeleteTextures(1, &_tbo);
  }
}

TexturedModel::~TexturedModel() {
  // free resources if sample model is loaded and enabled
  if (_loaded && !_disabled) {
    glDeleteVertexArrays(1, &_vao);
    glDeleteBuffers(1, &_vbo);
    glDeleteBuffers(1, &_ebo);
  }
}