#include "SampleGraphicModel.h"

#include "../Config.h"
#include "Vertex.h"
#include "juce_opengl/opengl/juce_gl.h"

using namespace juce::gl;

SampleGraphicModel::SampleGraphicModel(SamplePlayer *sp) {
  if (sp == nullptr || !sp->hasBeenInitialized()) {
    return;
  }

  _loaded = false;
  _disabled = false;

  // for now, all sample are simply rectangles (two triangles)
  // on which we map the fft texture.

  juce::Colour col = sp->getColor();

  _vertices.reserve(4);

  // upper left corner 0
  _vertices.push_back(
      {{float(sp->getEditingPosition()), -1.0f},
       {col.getFloatRed(), col.getFloatGreen(), col.getFloatBlue(), 0.0f},
       {0.0f, 1.0f}});

  // upper right corner 1
  _vertices.push_back(
      {{float(sp->getEditingPosition() + sp->getLength()), -1.0f},
       {col.getFloatRed(), col.getFloatGreen(), col.getFloatBlue(), 1.0f},
       {1.0f, 1.0f}});

  // lower right corner 2
  _vertices.push_back(
      {{float(sp->getEditingPosition() + sp->getLength()), 1.0f},
       {col.getFloatRed(), col.getFloatGreen(), col.getFloatBlue(), 1.0f},
       {1.0f, 0.0f}});

  // lower left corner 3
  _vertices.push_back(
      {{float(sp->getEditingPosition()), 1.0f},
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

  // generate texture from fft (layout described in SamplePlayer.h)
  std::vector<float> ffts = sp->getFftData();
  int numFfts = sp->getNumFft();
  int numChannels = sp->getBufferNumChannels();
  int channelSize = numFfts * FREQVIEW_SAMPLE_FFT_SCOPE_SIZE;

  // NOTE: we store the texture colors (fft intensity) as RGBA.
  // This implies some harsh data duplication, but openGL
  // requires texture lines to be 4bytes aligned and they
  // end up in this format anyway down the line.
  // We won't put the color in as we will mix it later in the
  // opengl shader with the vertex color.

  // NOTE: according to
  // https://gamedev.stackexchange.com/questions/133849/row-order-in-opengl-dxt-texture
  // which seems to quote an outdated version of the OpenGL doc, texture data
  // start from bottom left corner and fill the row left to right, then all the
  // rows above up to the final top right corner.

  // reserve the size of the displayed texture
  _textureHeight = 2 * FREQVIEW_SAMPLE_FFT_SCOPE_SIZE;
  _textureWidth = numFfts;
  _texture.resize(_textureHeight * _textureWidth * 4);  // 4 is for rgba values

  float intensity = 0.0f;

  int texturePos = 0;

  // for each fourier transform over time
  for (int i = 0; i < numFfts; i++) {
    // for each intensity in the fft scope
    for (int j = 0; j < FREQVIEW_SAMPLE_FFT_SCOPE_SIZE; j++) {
      intensity = ffts[(i * FREQVIEW_SAMPLE_FFT_SCOPE_SIZE) + j];
      // scale intensity to be prettier and in bound
      intensity = juce::jmap(intensity, MIN_DB, MAX_DB, 0.0f, 1.0f);
      intensity = sigmoid((intensity * 12.0) - 6.0);
      intensity = juce::jlimit(0.0f, 1.0f, intensity);
      // find coordinate inside the damn texture
      texturePos = ((numFfts * j) + i) * 4;
      _texture[texturePos] = 1.0f;           // R
      _texture[texturePos + 1] = 1.0f;       // G
      _texture[texturePos + 2] = 1.0f;       // B
      _texture[texturePos + 3] = intensity;  // A

      // for the other channel
      texturePos =
          channelSize +
          ((numFfts * (FREQVIEW_SAMPLE_FFT_SCOPE_SIZE - (j + 1))) + i) * 4;

      // if there's only one channel, mirror the data
      if (numChannels == 1) {
        _texture[texturePos] = 1.0f;           // R
        _texture[texturePos + 1] = 1.0f;       // G
        _texture[texturePos + 2] = 1.0f;       // B
        _texture[texturePos + 3] = intensity;  // A
      } else {
        // else, numchannels must be two, we pick the other channel
        // data and assign it
        intensity =
            ffts[channelSize + (i * FREQVIEW_SAMPLE_FFT_SCOPE_SIZE) + j];
        // scale intensity to be prettier and in bound
        intensity = juce::jmap(intensity, MIN_DB, MAX_DB, 0.0f, 1.0f);
        intensity = sigmoid((intensity * 12.0) - 6.0);
        intensity = juce::jlimit(0.0f, 1.0f, intensity);
        // assign the values
        _texture[texturePos] = 1.0f;           // R
        _texture[texturePos + 1] = 1.0f;       // G
        _texture[texturePos + 2] = 1.0f;       // B
        _texture[texturePos + 3] = intensity;  // A
      }
    }
  }

  _textureBytes.reserve(_textureHeight * _textureWidth * 4);
  for (int i = 0; i < (_textureHeight * _textureWidth); i++) {
    int basepos = i * 4;
    _textureBytes[basepos] = 255;
    _textureBytes[basepos + 1] = 0;
    _textureBytes[basepos + 2] = 0;
    _textureBytes[basepos + 3] = 255;
  }
}

SampleGraphicModel::~SampleGraphicModel() {
  // free resources if sample model is loaded and enabled
  if (_loaded && !_disabled) {
    glDeleteVertexArrays(1, &_vao);
    glDeleteBuffers(1, &_vbo);
    glDeleteBuffers(1, &_ebo);
    glDeleteTextures(1, &_tbo);
  }
}

// registerGlObjects registers vertices, triangle indices and textures.
// Warning, call it from the openGL thread using the juce opengl context
// utility for that !
void SampleGraphicModel::registerGlObjects() {
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
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, _textureWidth, _textureHeight, 0,
               GL_UNSIGNED_BYTE, GL_RGBA, _texture.data());
  glGenerateMipmap(GL_TEXTURE_2D);

  _loaded = true;
}

// drawGlObjects will draw the sample. Watch out, it needs to be called
// from the openGL drawing function of juce OpenGL context
void SampleGraphicModel::drawGlObjects() {
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

void SampleGraphicModel::disable() {
  if (!_disabled && _loaded) {
    _disabled = true;
    glDeleteVertexArrays(1, &_vao);
    glDeleteBuffers(1, &_vbo);
    glDeleteBuffers(1, &_ebo);
    // glDeleteTextures(1, &_tbo);
  }
}

float SampleGraphicModel::sigmoid(float val) { return 1 / (1 + exp(-val)); }