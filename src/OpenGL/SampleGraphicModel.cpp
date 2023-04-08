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
  int freqiZoomed = 0;

  // for each fourier transform over time
  for (int ffti = 0; ffti < numFfts; ffti++) {
    // for each frequency of the current fourrier transform
    for (int freqi = 0; freqi < FREQVIEW_SAMPLE_FFT_SCOPE_SIZE; freqi++) {
      // with this texture position, freqi goes from top to bottom
      // and ffti goes from left to right onscreen.
      texturePos = ((freqi * numFfts) + ffti) * 4;
      // we apply our polynomial lens freqi transformation to zoom in a bit
      freqiZoomed = _transformFrequencyLocation(freqi);
      // as the frequencies in the ffts goes from low to high, we have
      // to flip the freqi to fetch the frequency and it's all good !
      intensity = ffts[(ffti * FREQVIEW_SAMPLE_FFT_SCOPE_SIZE) +
                       (FREQVIEW_SAMPLE_FFT_SCOPE_SIZE - (freqi + 1))];
      // increase contrast and map between 0 and 1
      _transformIntensity(intensity);
      // now we write the intensity into the texture
      _texture[texturePos] = col.getFloatRed();
      _texture[texturePos + 1] = col.getFloatGreen();
      _texture[texturePos + 2] = col.getFloatBlue();
      _texture[texturePos + 3] = intensity;
    }
  }
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