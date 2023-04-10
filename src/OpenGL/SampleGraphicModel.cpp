#include "SampleGraphicModel.h"

#include "../Config.h"
#include "Vertex.h"

using namespace juce::gl;

SampleGraphicModel::SampleGraphicModel(SamplePlayer* sp) {
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

  // the texture scaling in opengl use either manhatan distance or linear sum
  // of neigbouring pixels. So as we have less pixel over time than pixel over
  // frequencies, a glitch appear and the values leak to the sides.
  // This multiplier should help reduce horizontal leakage of textures.
  int horizontalScaleMultiplier = 16;

  // NOTE: we store the texture colors (fft intensity) as RGBA.
  // This implies some harsh data duplication, but openGL
  // requires texture lines to be 4bytes aligned and they
  // end up in this format anyway down the line.
  // We won't put the color in as we will mix it later in the
  // opengl shader with the vertex color.

  // NOTE: according to
  // https://gamedev.stackexchange.com/questions/133849/row-order-in-opengl-dxt-texture
  // which seems to quote an outdated version of the OpenGL doc, texture
  // data start from bottom left corner and fill the row left to right, then
  // all the rows above up to the final top right corner.

  // reserve the size of the displayed texture
  _textureHeight = 2 * FREQVIEW_SAMPLE_FFT_SCOPE_SIZE;
  _textureWidth = numFfts * horizontalScaleMultiplier;
  _texture.resize(_textureHeight * _textureWidth * 4);  // 4 is for rgba values
  std::fill(_texture.begin(), _texture.end(), 1.0f);

  float intensity = 0.0f;

  int texturePos = 0;
  int freqiZoomed = 0;

  int channelTextureShift = _textureWidth * (_textureHeight >> 1) * 4;
  int channelFftsShift = numFfts * FREQVIEW_SAMPLE_FFT_SCOPE_SIZE;

  // for each fourier transform over time
  for (int ffti = 0; ffti < numFfts; ffti++) {
    // for each frequency of the current fourrier transform
    for (int freqi = 0; freqi < FREQVIEW_SAMPLE_FFT_SCOPE_SIZE; freqi++) {
      // we apply our polynomial lens freqi transformation to zoom in a bit
      freqiZoomed = _transformFrequencyLocation(freqi);
      // as the frequencies in the ffts goes from low to high, we have
      // to flip the freqi to fetch the frequency and it's all good !
      intensity = ffts[(ffti * FREQVIEW_SAMPLE_FFT_SCOPE_SIZE) +
                       (FREQVIEW_SAMPLE_FFT_SCOPE_SIZE - (freqiZoomed + 1))];
      // increase contrast and map between 0 and 1
      _transformIntensity(intensity);

      for (int nDuplicate = 0; nDuplicate < horizontalScaleMultiplier;
           nDuplicate++) {
        // with this texture position, freqi goes from top to bottom
        // and ffti goes from left to right onscreen.
        texturePos = ((freqi * numFfts * horizontalScaleMultiplier) +
                      (nDuplicate + (ffti * horizontalScaleMultiplier))) *
                     4;
        // now we write the intensity into the texture
        _texture[texturePos] = col.getFloatRed();
        _texture[texturePos + 1] = col.getFloatGreen();
        _texture[texturePos + 2] = col.getFloatBlue();
        _texture[texturePos + 3] = intensity;
      }

      // now we write the other channel on bottom part (if not exists, write
      // first channel instead)

      // pick freq index in the fft
      freqiZoomed = FREQVIEW_SAMPLE_FFT_SCOPE_SIZE -
                    (_transformFrequencyLocation(
                         (FREQVIEW_SAMPLE_FFT_SCOPE_SIZE - (freqi + 1))) +
                     1);
      // get the value depending on if we got a second channel or not
      if (numChannels == 2) {
        intensity = ffts[channelFftsShift +
                         (ffti * FREQVIEW_SAMPLE_FFT_SCOPE_SIZE) + freqiZoomed];
      } else {
        intensity = ffts[(ffti * FREQVIEW_SAMPLE_FFT_SCOPE_SIZE) + freqiZoomed];
      }
      _transformIntensity(intensity);

      for (int nDuplicate = 0; nDuplicate < horizontalScaleMultiplier;
           nDuplicate++) {
        texturePos = channelTextureShift +
                     ((freqi * numFfts * horizontalScaleMultiplier) +
                      (nDuplicate + (ffti * horizontalScaleMultiplier))) *
                         4;
        _texture[texturePos] = col.getFloatRed();
        _texture[texturePos + 1] = col.getFloatGreen();
        _texture[texturePos + 2] = col.getFloatBlue();
        _texture[texturePos + 3] = intensity;
      }
    }
  }
}

// To run on opengl thread, will move track to this absolute editing position in
// audio frames.
void SampleGraphicModel::move(int64_t position) {
  int width = _vertices[1].position[0] - _vertices[0].position[0];

  _vertices[0].position[0] = position;
  _vertices[1].position[0] = position + width;
  _vertices[2].position[0] = position + width;
  _vertices[3].position[0] = position;

  uploadVerticesToGpu();
}

void SampleGraphicModel::_transformIntensity(float& intensity) {
  // we need to normalize the frequency by mapping the range
  intensity = juce::jmap(intensity, MIN_DB, MAX_DB, 0.0f, 1.0f);
  // then we make it a little prettier with a sigmoid function
  // (increase contrasts)
  intensity = sigmoid((intensity * 12.0) - 6.0);
  // and finally we make sure it falls in range
  intensity = juce::jlimit(0.0f, 1.0f, intensity);
}

int SampleGraphicModel::_transformFrequencyLocation(int freqi) {
  // we apply our polynomial lens freqi transformation to zoom in a bit
  int freqiZoomed =
      int(polylens(float(freqi) / float(FREQVIEW_SAMPLE_FFT_SCOPE_SIZE)) *
          FREQVIEW_SAMPLE_FFT_SCOPE_SIZE);
  // let's take extra care that it's inbound
  freqiZoomed =
      juce::jlimit(0, FREQVIEW_SAMPLE_FFT_SCOPE_SIZE - 1, freqiZoomed);
  return freqiZoomed;
}

// Save position and width when selection dragging begins.
void SampleGraphicModel::initDrag() {
  _dragStartPosition = _vertices[0].position[0];
  _lastWidth = _vertices[1].position[0] - _vertices[0].position[0];
}

// To run on opengl thread, will update track position to account
// for the new shift during selection dragging.
void SampleGraphicModel::updateDrag(int frameMove) {
  int position = _dragStartPosition + frameMove;
  _vertices[0].position[0] = position;
  _vertices[1].position[0] = position + _lastWidth;
  _vertices[2].position[0] = position + _lastWidth;
  _vertices[3].position[0] = position;
  uploadVerticesToGpu();
}

void SampleGraphicModel::uploadVerticesToGpu() {
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
}