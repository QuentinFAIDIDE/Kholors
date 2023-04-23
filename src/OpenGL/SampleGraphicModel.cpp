#include "SampleGraphicModel.h"

#include "../Config.h"
#include "Vertex.h"
#include <algorithm>

using namespace juce::gl;

#include "../Audio/UnitConverter.h"

SampleGraphicModel::SampleGraphicModel(SamplePlayer *sp, juce::Colour col)
{
    if (sp == nullptr || !sp->hasBeenInitialized())
    {
        return;
    }

    loaded = false;
    disabled = false;

    // for now, all sample are simply rectangles (two triangles)
    // on which we map the fft texture.

    color = col;

    updatePropertiesAndUploadToGpu(sp);

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
    texture.resize(textureHeight * textureWidth * 4); // 4 is for rgba values
    std::fill(texture.begin(), texture.end(), 1.0f);

    float intensity = 0.0f;

    int texturePos = 0;
    int freqiZoomed = 0;

    channelTextureShift = textureWidth * (textureHeight >> 1) * 4;
    int channelFftsShift = numFfts * FREQVIEW_SAMPLE_FFT_SCOPE_SIZE;

    // for each fourier transform over time
    for (int ffti = 0; ffti < numFfts; ffti++)
    {
        // for each frequency of the current fourrier transform
        for (int freqi = 0; freqi < FREQVIEW_SAMPLE_FFT_SCOPE_SIZE; freqi++)
        {
            // we apply our polynomial lens freqi transformation to zoom in a bit
            freqiZoomed = UnitConverter::magnifyTextureFrequencyIndex(freqi);
            // as the frequencies in the ffts goes from low to high, we have
            // to flip the freqi to fetch the frequency and it's all good !
            intensity =
                ffts[(ffti * FREQVIEW_SAMPLE_FFT_SCOPE_SIZE) + (FREQVIEW_SAMPLE_FFT_SCOPE_SIZE - (freqiZoomed + 1))];
            // increase contrast and map between 0 and 1
            intensity = UnitConverter::magnifyIntensity(intensity);

            for (int nDuplicate = 0; nDuplicate < horizontalScaleMultiplier; nDuplicate++)
            {
                texturePos = getTextureIndex(freqi, ffti, nDuplicate, true);
                // now we write the intensity into the texture
                texture[texturePos] = 1.0f;
                texture[texturePos + 1] = 1.0f;
                texture[texturePos + 2] = 1.0f;
                texture[texturePos + 3] = intensity;
            }

            // now we write the other channel on bottom part (if not exists, write
            // first channel instead)

            // pick freq index in the fft
            freqiZoomed =
                FREQVIEW_SAMPLE_FFT_SCOPE_SIZE -
                (UnitConverter::magnifyTextureFrequencyIndex((FREQVIEW_SAMPLE_FFT_SCOPE_SIZE - (freqi + 1))) + 1);
            // get the value depending on if we got a second channel or not
            if (numChannels == 2)
            {
                intensity = ffts[channelFftsShift + (ffti * FREQVIEW_SAMPLE_FFT_SCOPE_SIZE) + freqiZoomed];
            }
            else
            {
                intensity = ffts[(ffti * FREQVIEW_SAMPLE_FFT_SCOPE_SIZE) + freqiZoomed];
            }
            intensity = UnitConverter::magnifyIntensity(intensity);

            for (int nDuplicate = 0; nDuplicate < horizontalScaleMultiplier; nDuplicate++)
            {
                texturePos = getTextureIndex(freqi, ffti, nDuplicate, false);
                texture[texturePos] = 1.0f;
                texture[texturePos + 1] = 1.0f;
                texture[texturePos + 2] = 1.0f;
                texture[texturePos + 3] = intensity;
            }
        }
    }
}

void SampleGraphicModel::updatePropertiesAndUploadToGpu(SamplePlayer *sp)
{

    startPositionNormalized = float(sp->getBufferStart()) / float(sp->getTotalLength());
    endPositionNormalised = float(sp->getBufferEnd()) / float(sp->getTotalLength());

    float leftX = float(sp->getEditingPosition());
    float rightX = leftX + float(sp->getLength());

    generateAndUploadVertices(leftX, rightX);
}

void SampleGraphicModel::generateAndUploadVertices(float leftX, float rightX)
{

    vertices.reserve(8);
    triangleIds.reserve(6);
    vertices.clear();
    triangleIds.clear();

    // upper left corner 0
    vertices.push_back({{leftX, -1.0f},
                        {color.getFloatRed(), color.getFloatGreen(), color.getFloatBlue(), 1.0f},
                        {startPositionNormalized, 1.0f}});

    // upper right corner 1
    vertices.push_back({{rightX, -1.0f},
                        {color.getFloatRed(), color.getFloatGreen(), color.getFloatBlue(), 1.0f},
                        {endPositionNormalised, 1.0f}});

    // right upper band bottom corner 2
    vertices.push_back({{rightX, 0.0f},
                        {color.getFloatRed(), color.getFloatGreen(), color.getFloatBlue(), 1.0f},
                        {endPositionNormalised, 0.5f}});

    // left upper band bottom corner 3
    vertices.push_back({{leftX, 0.0f},
                        {color.getFloatRed(), color.getFloatGreen(), color.getFloatBlue(), 1.0f},
                        {startPositionNormalized, 0.5f}});

    // left lower band top 4
    vertices.push_back({{leftX, 0.0f},
                        {color.getFloatRed(), color.getFloatGreen(), color.getFloatBlue(), 1.0f},
                        {startPositionNormalized, 0.5f}});

    // right lower band top 5
    vertices.push_back({{rightX, 0.0f},
                        {color.getFloatRed(), color.getFloatGreen(), color.getFloatBlue(), 1.0f},
                        {endPositionNormalised, 0.5f}});

    // lower right corner 6
    vertices.push_back({{rightX, 1.0f},
                        {color.getFloatRed(), color.getFloatGreen(), color.getFloatBlue(), 1.0f},
                        {endPositionNormalised, 0.0f}});

    // lower left corner 7
    vertices.push_back({{leftX, 1.0f},
                        {color.getFloatRed(), color.getFloatGreen(), color.getFloatBlue(), 1.0f},
                        {startPositionNormalized, 0.0f}});

    connectSquareFromVertexIds(0, 1, 2, 3);
    connectSquareFromVertexIds(4, 5, 6, 7);
    uploadVerticesToGpu();
}

void SampleGraphicModel::connectSquareFromVertexIds(size_t topLeft, size_t topRight, size_t bottomRight,
                                                    size_t bottomLeft)
{
    // lower left triangle
    triangleIds.push_back(topLeft);
    triangleIds.push_back(bottomRight);
    triangleIds.push_back(bottomLeft);

    // upper right triangle
    triangleIds.push_back(topLeft);
    triangleIds.push_back(topRight);
    triangleIds.push_back(bottomRight);
}

int SampleGraphicModel::getTextureIndex(int freqIndex, int timeIndex, int freqDuplicateShift, bool isLeftChannel)
{
    if (isLeftChannel)
    {
        return ((freqIndex * numFfts * horizontalScaleMultiplier) +
                (freqDuplicateShift + (timeIndex * horizontalScaleMultiplier))) *
               4;
    }
    else
    {
        return channelTextureShift + ((freqIndex * numFfts * horizontalScaleMultiplier) +
                                      (freqDuplicateShift + (timeIndex * horizontalScaleMultiplier))) *
                                         4;
    }
}

// To run on opengl thread, will recolor track
void SampleGraphicModel::setColor(juce::Colour &col)
{
    float leftX = vertices[0].position[0];
    float rightX = vertices[1].position[0];
    color = col;
    generateAndUploadVertices(leftX, rightX);
}

// Save position and width when selection dragging begins.
void SampleGraphicModel::initDrag()
{
    dragStartPosition = vertices[0].position[0];
    lastWidth = vertices[1].position[0] - vertices[0].position[0];
}

// To run on opengl thread, will update track position to account
// for the new shift during selection dragging.
void SampleGraphicModel::updateDrag(int frameMove)
{
    int position = dragStartPosition + frameMove;
    generateAndUploadVertices(position, position + lastWidth);
}

void SampleGraphicModel::uploadVerticesToGpu()
{
    glBindVertexArray(vao);

    // register and upload the vertices data
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), vertices.data(), GL_STATIC_DRAW);
    // register and upload indices of the vertices to form the triangles
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * triangleIds.size(), triangleIds.data(),
                 GL_STATIC_DRAW);
}

float SampleGraphicModel::textureIntensity(float x, float y)
{
    float xInAudioBuffer = juce::jmap(x, startPositionNormalized, endPositionNormalised);
    int timeIndex = xInAudioBuffer * numFfts;
    // index of zoomed frequencies, not linear to logarithm of frequencies
    int freqIndexNormalised = 0;
    if (y < 0.5)
    {
        freqIndexNormalised = y * 2 * FREQVIEW_SAMPLE_FFT_SCOPE_SIZE;
    }
    else
    {
        freqIndexNormalised = (y - 0.5) * 2 * FREQVIEW_SAMPLE_FFT_SCOPE_SIZE;
    }
    return texture[getTextureIndex(freqIndexNormalised, timeIndex, 1, y < 0.5) + 3];
}

juce::int64 SampleGraphicModel::getFramePosition()
{
    return juce::int64(vertices[0].position[0]);
}

juce::int64 SampleGraphicModel::getFrameLength()
{
    return juce::int64(vertices[1].position[0] - vertices[0].position[0]);
}