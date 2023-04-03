#include "RectangleModel.h"

#include "juce_opengl/opengl/juce_gl.h"

using namespace juce::gl;

RectangleModel::RectangleModel(juce::Colour col, float x, float y, float width,
                               float height) {
  _loaded = false;
  _disabled = false;

  _vertices.reserve(4);

  // upper left corner 0
  _vertices.push_back(
      {{x, y, 0, 0},
       {col.getFloatRed(), col.getFloatGreen(), col.getFloatBlue(), 0.0f}});

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
  for (int ffti = 0; ffti < numFfts; ffti++) {
    for (int freqi = 0; freqi < FREQVIEW_SAMPLE_FFT_SCOPE_SIZE; freqi++) {
      // with this texture position, freqi goes from top to bottom
      // and ffti goes from left to right onscreen.
      texturePos = ((freqi * numFfts) + ffti) * 4;
      // as the frequencies in the ffts goes from low to high, we have
      // to flip the freqi to fetch the frequency and it's all good !
      intensity = ffts[(ffti * FREQVIEW_SAMPLE_FFT_SCOPE_SIZE) +
                       (FREQVIEW_SAMPLE_FFT_SCOPE_SIZE - (freqi + 1))];
      // we need to normalize the frequency by mapping the range
      intensity = juce::jmap(intensity, MIN_DB, MAX_DB, 0.0f, 1.0f);
      // then we make it a little prettier with a sigmoid function
      // (increase contrasts)
      intensity = sigmoid((intensity * 12.0) - 6.0);
      // and finally we make sure it falls in range
      intensity = juce::jlimit(0.0f, 1.0f, intensity);
      // now we write the intensity into the texture
      _texture[texturePos] = col.getFloatRed();
      _texture[texturePos + 1] = col.getFloatGreen();
      _texture[texturePos + 2] = col.getFloatBlue();
      _texture[texturePos + 3] = intensity;
    }
  }
}

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