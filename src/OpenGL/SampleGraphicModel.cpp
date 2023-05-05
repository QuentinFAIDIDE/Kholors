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

    // stored fft data we parse in SamplePlayer
    std::vector<float> ffts = sp->getFftData();
    loadFftDataToTexture(ffts, sp->getNumFft(), sp->getBufferNumChannels());
}

void SampleGraphicModel::loadFftDataToTexture(std::vector<float> &ffts, int fftCount, int channelCount)
{
    numFfts = fftCount;
    numChannels = channelCount;

    // the texture scaling in opengl use either manhatan distance or linear sum
    // of neigbouring pixels. So as we have less pixel over time than pixel over
    // frequencies, a glitch appear and the values leak to the sides.
    // This multiplier should help reduce horizontal leakage of textures.
    // If this is raised too high, this can blow up the GPU memory real fast.
    // If you change this value, make sure to stress test the loading a bit on shitty GPUs.
    horizontalScaleMultiplier = 4;

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
        // for each frequency of the texture (linear to displayed texture)
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

    lastLowPassFreq = sp->getLowPassFreq();
    lastHighPassFreq = sp->getHighPassFreq();

    generateAndUploadVertices(leftX, rightX, lastLowPassFreq, lastHighPassFreq);
}

void SampleGraphicModel::generateAndUploadVertices(float leftX, float rightX, float lowPassFreq, float highPassFreq)
{

    vertices.reserve(8);
    triangleIds.reserve(6);
    vertices.clear();
    triangleIds.clear();

    float lowPassPositionRatio = UnitConverter::freqToPositionRatio(lowPassFreq);
    float highPassPositionRatio = UnitConverter::freqToPositionRatio(highPassFreq);
    float halfLowPassPos = lowPassPositionRatio / 2.0;
    float halfHighPassPos = highPassPositionRatio / 2.0;

    // upper left corner 0
    vertices.push_back({{leftX, -lowPassPositionRatio},
                        {color.getFloatRed(), color.getFloatGreen(), color.getFloatBlue(), 1.0f},
                        {startPositionNormalized, 0.5f + halfLowPassPos}});

    // upper right corner 1
    vertices.push_back({{rightX, -lowPassPositionRatio},
                        {color.getFloatRed(), color.getFloatGreen(), color.getFloatBlue(), 1.0f},
                        {endPositionNormalised, 0.5f + halfLowPassPos}});

    // right upper band bottom corner 2
    vertices.push_back({{rightX, -highPassPositionRatio},
                        {color.getFloatRed(), color.getFloatGreen(), color.getFloatBlue(), 1.0f},
                        {endPositionNormalised, 0.5f + halfHighPassPos}});

    // left upper band bottom corner 3
    vertices.push_back({{leftX, -highPassPositionRatio},
                        {color.getFloatRed(), color.getFloatGreen(), color.getFloatBlue(), 1.0f},
                        {startPositionNormalized, 0.5f + halfHighPassPos}});

    // left lower band top 4
    vertices.push_back({{leftX, highPassPositionRatio},
                        {color.getFloatRed(), color.getFloatGreen(), color.getFloatBlue(), 1.0f},
                        {startPositionNormalized, 0.5f - halfHighPassPos}});

    // right lower band top 5
    vertices.push_back({{rightX, highPassPositionRatio},
                        {color.getFloatRed(), color.getFloatGreen(), color.getFloatBlue(), 1.0f},
                        {endPositionNormalised, 0.5f - halfHighPassPos}});

    // lower right corner 6
    vertices.push_back({{rightX, lowPassPositionRatio},
                        {color.getFloatRed(), color.getFloatGreen(), color.getFloatBlue(), 1.0f},
                        {endPositionNormalised, 0.5f - halfLowPassPos}});

    // lower left corner 7
    vertices.push_back({{leftX, lowPassPositionRatio},
                        {color.getFloatRed(), color.getFloatGreen(), color.getFloatBlue(), 1.0f},
                        {startPositionNormalized, 0.5f - halfLowPassPos}});

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
    generateAndUploadVertices(leftX, rightX, lastLowPassFreq, lastHighPassFreq);
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
    generateAndUploadVertices(position, position + lastWidth, lastLowPassFreq, lastHighPassFreq);
}

void SampleGraphicModel::uploadVerticesToGpu()
{

    const juce::ScopedLock lock(loadingMutex);

    glBindVertexArray(vao);

    // register and upload the vertices data
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), vertices.data(), GL_STATIC_DRAW);
    // register and upload indices of the vertices to form the triangles
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * triangleIds.size(), triangleIds.data(),
                 GL_STATIC_DRAW);

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
    {
        std::cerr << "got following open gl error after upploading vertices data: " << err << std::endl;
    }
}

float SampleGraphicModel::textureIntensity(float x, float y)
{
    if (x < 0.0 || x > 1.0 || y < 0.0 || y > 1.0)
    {
        return 0.0f;
    }

    if (isFilteredArea(y))
    {
        return 0.0f;
    }

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

int SampleGraphicModel::isFilteredArea(float y)
{
    // Here, the "top" is in the sense of the frequency, i.e., on-screen position.
    // The "top" value is therefore lower in value to that of the "bottom" in the
    // coordinates system (taken from the Juce library) !

    float leftChannelTop = 0.5 - (0.5 * UnitConverter::freqToPositionRatio(lastLowPassFreq));
    float leftChannelBottom = 0.5 - (0.5 * UnitConverter::freqToPositionRatio(lastHighPassFreq));

    float rightChannelTop = 0.5 + (0.5 * UnitConverter::freqToPositionRatio(lastHighPassFreq));
    float rightChannelBottom = 0.5 + (0.5 * UnitConverter::freqToPositionRatio(lastLowPassFreq));

    if (y < leftChannelTop || (y > leftChannelBottom && y < rightChannelTop) || y > rightChannelBottom)
    {
        return true;
    }
    return false;
}

juce::int64 SampleGraphicModel::getFramePosition()
{
    return juce::int64(vertices[0].position[0]);
}

juce::int64 SampleGraphicModel::getFrameLength()
{
    return juce::int64(vertices[1].position[0] - vertices[0].position[0]);
}

std::vector<juce::Rectangle<float>> SampleGraphicModel::getPixelBounds(float viewPosition, float viewScale,
                                                                       float viewHeight)
{
    float freqRatioLowPass = UnitConverter::freqToPositionRatio(lastLowPassFreq);
    float freqRatioHighPass = UnitConverter::freqToPositionRatio(lastHighPassFreq);
    float height = (freqRatioLowPass - freqRatioHighPass) * viewHeight;

    std::vector<juce::Rectangle<float>> rectangles;

    rectangles.push_back(juce::Rectangle<float>(
        (vertices[0].position[0] - viewPosition) / viewScale, (1.0 - freqRatioLowPass) * (viewHeight / 2.0),
        (vertices[1].position[0] - vertices[0].position[0]) / viewScale, height / 2.0));

    rectangles.push_back(juce::Rectangle<float>((vertices[0].position[0] - viewPosition) / viewScale,
                                                (viewHeight / 2.0) + ((freqRatioHighPass) * (viewHeight / 2.0)),
                                                (vertices[1].position[0] - vertices[0].position[0]) / viewScale,
                                                height / 2.0));

    return rectangles;
}
