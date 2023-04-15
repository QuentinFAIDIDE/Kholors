#include "SampleGraphicModel.h"

#include "../Config.h"
#include "Vertex.h"

using namespace juce::gl;

SampleGraphicModel::SampleGraphicModel(SamplePlayer* sp) {
  if (sp == nullptr || !sp->hasBeenInitialized()) {
    return;
  }

  loaded = false;
  disabled = false;

  // for now, all sample are simply rectangles (two triangles)
  // on which we map the fft texture.

  juce::Colour col = sp->getColor();

  vertices.reserve(4);

  // upper left corner 0
  vertices.push_back(
      {{float(sp->getEditingPosition()), -1.0f},
       {col.getFloatRed(), col.getFloatGreen(), col.getFloatBlue(), 0.0f},
       {0.0f, 1.0f}});

  // upper right corner 1
  vertices.push_back(
      {{float(sp->getEditingPosition() + sp->getLength()), -1.0f},
       {col.getFloatRed(), col.getFloatGreen(), col.getFloatBlue(), 1.0f},
       {1.0f, 1.0f}});

  // lower right corner 2
  vertices.push_back(
      {{float(sp->getEditingPosition() + sp->getLength()), 1.0f},
       {col.getFloatRed(), col.getFloatGreen(), col.getFloatBlue(), 1.0f},
       {1.0f, 0.0f}});

  // lower left corner 3
  vertices.push_back(
      {{float(sp->getEditingPosition()), 1.0f},
       {col.getFloatRed(), col.getFloatGreen(), col.getFloatBlue(), 1.0f},
       {0.0f, 0.0f}});

  // lower left triangle
  triangleIds.push_back(0);
  triangleIds.push_back(2);
  triangleIds.push_back(3);

  // upper right triangle
  triangleIds.push_back(0);
  triangleIds.push_back(1);
  triangleIds.push_back(2);

  // generate texture from fft (layout described in SamplePlayer.h)
  std::vector<float> ffts = sp->getFftData();
  numFfts = sp->getNumFft();
  numChannels = sp->getBufferNumChannels();

  // the texture scaling in opengl use either manhatan distance or linear sum
  // of neigbouring pixels. So as we have less pixel over time than pixel over
  // frequencies, a glitch appear and the values leak to the sides.
  // This multiplier should help reduce horizontal leakage of textures.
  horizontalScaleMultiplier = 16;

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
  textureHeight = 2 * FREQVIEW_SAMPLE_FFT_SCOPE_SIZE;
  textureWidth = numFfts * horizontalScaleMultiplier;
  texture.resize(textureHeight * textureWidth * 4);  // 4 is for rgba values
  std::fill(texture.begin(), texture.end(), 1.0f);

  float intensity = 0.0f;

  int texturePos = 0;
  int freqiZoomed = 0;

  channelTextureShift = textureWidth * (textureHeight >> 1) * 4;
  int channelFftsShift = numFfts * FREQVIEW_SAMPLE_FFT_SCOPE_SIZE;

  // for each fourier transform over time
  for (int ffti = 0; ffti < numFfts; ffti++) {
    // for each frequency of the current fourrier transform
    for (int freqi = 0; freqi < FREQVIEW_SAMPLE_FFT_SCOPE_SIZE; freqi++) {
      // we apply our polynomial lens freqi transformation to zoom in a bit
      freqiZoomed = transformFrequencyLocation(freqi);
      // as the frequencies in the ffts goes from low to high, we have
      // to flip the freqi to fetch the frequency and it's all good !
      intensity = ffts[(ffti * FREQVIEW_SAMPLE_FFT_SCOPE_SIZE) +
                       (FREQVIEW_SAMPLE_FFT_SCOPE_SIZE - (freqiZoomed + 1))];
      // increase contrast and map between 0 and 1
      transformIntensity(intensity);

      for (int nDuplicate = 0; nDuplicate < horizontalScaleMultiplier;
           nDuplicate++) {
        texturePos = getTextureIndex(freqi, ffti, nDuplicate, true);
        // now we write the intensity into the texture
        texture[texturePos] = col.getFloatRed();
        texture[texturePos + 1] = col.getFloatGreen();
        texture[texturePos + 2] = col.getFloatBlue();
        texture[texturePos + 3] = intensity;
      }

      // now we write the other channel on bottom part (if not exists, write
      // first channel instead)

      // pick freq index in the fft
      freqiZoomed = FREQVIEW_SAMPLE_FFT_SCOPE_SIZE -
                    (transformFrequencyLocation(
                         (FREQVIEW_SAMPLE_FFT_SCOPE_SIZE - (freqi + 1))) +
                     1);
      // get the value depending on if we got a second channel or not
      if (numChannels == 2) {
        intensity = ffts[channelFftsShift +
                         (ffti * FREQVIEW_SAMPLE_FFT_SCOPE_SIZE) + freqiZoomed];
      } else {
        intensity = ffts[(ffti * FREQVIEW_SAMPLE_FFT_SCOPE_SIZE) + freqiZoomed];
      }
      transformIntensity(intensity);

      for (int nDuplicate = 0; nDuplicate < horizontalScaleMultiplier;
           nDuplicate++) {
        texturePos = getTextureIndex(freqi, ffti, nDuplicate, false);
        texture[texturePos] = col.getFloatRed();
        texture[texturePos + 1] = col.getFloatGreen();
        texture[texturePos + 2] = col.getFloatBlue();
        texture[texturePos + 3] = intensity;
      }
    }
  }
}

int SampleGraphicModel::getTextureIndex(int freqIndex, int timeIndex,
                                        int freqDuplicateShift,
                                        bool isLeftChannel) {
  if (isLeftChannel) {
    return ((freqIndex * numFfts * horizontalScaleMultiplier) +
            (freqDuplicateShift + (timeIndex * horizontalScaleMultiplier))) *
           4;
  } else {
    return channelTextureShift +
           ((freqIndex * numFfts * horizontalScaleMultiplier) +
            (freqDuplicateShift + (timeIndex * horizontalScaleMultiplier))) *
               4;
  }
}

// To run on opengl thread, will move track to this absolute editing position in
// audio frames.
void SampleGraphicModel::move(int64_t position) {
  int width = vertices[1].position[0] - vertices[0].position[0];

  vertices[0].position[0] = position;
  vertices[1].position[0] = position + width;
  vertices[2].position[0] = position + width;
  vertices[3].position[0] = position;

  uploadVerticesToGpu();
}

void SampleGraphicModel::transformIntensity(float& intensity) {
  // we need to normalize the frequency by mapping the range
  intensity = juce::jmap(intensity, MIN_DB, MAX_DB, 0.0f, 1.0f);
  // then we make it a little prettier with a sigmoid function
  // (increase contrasts)
  intensity = sigmoid((intensity * 12.0) - 6.0);
  // and finally we make sure it falls in range
  intensity = juce::jlimit(0.0f, 1.0f, intensity);
}

int SampleGraphicModel::transformFrequencyLocation(int freqi) {
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
  dragStartPosition = vertices[0].position[0];
  lastWidth = vertices[1].position[0] - vertices[0].position[0];
}

// To run on opengl thread, will update track position to account
// for the new shift during selection dragging.
void SampleGraphicModel::updateDrag(int frameMove) {
  int position = dragStartPosition + frameMove;
  vertices[0].position[0] = position;
  vertices[1].position[0] = position + lastWidth;
  vertices[2].position[0] = position + lastWidth;
  vertices[3].position[0] = position;
  uploadVerticesToGpu();
}

void SampleGraphicModel::uploadVerticesToGpu() {
  glBindVertexArray(vao);

  // register and upload the vertices data
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(),
               vertices.data(), GL_STATIC_DRAW);
  // register and upload indices of the vertices to form the triangles
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               sizeof(unsigned int) * triangleIds.size(), triangleIds.data(),
               GL_STATIC_DRAW);
}

float SampleGraphicModel::textureIntensity(float x, float y) {
  int timeIndex = x * numFfts;
  // index of zoomed frequencies, not linear to logarithm of frequencies
  int freqIndexNormalised = 0;
  if (y < 0.5) {
    freqIndexNormalised = y * 2 * FREQVIEW_SAMPLE_FFT_SCOPE_SIZE;
  } else {
    freqIndexNormalised = (y - 0.5) * 2 * FREQVIEW_SAMPLE_FFT_SCOPE_SIZE;
  }
  return texture[getTextureIndex(freqIndexNormalised, timeIndex, 1, y < 0.5) +
                 3];
}

juce::int64 SampleGraphicModel::getFramePosition() {
  return juce::int64(vertices[0].position[0]);
}

juce::int64 SampleGraphicModel::getFrameLength() {
  return juce::int64(vertices[1].position[0] - vertices[0].position[0]);
}