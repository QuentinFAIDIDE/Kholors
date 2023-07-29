#include "SampleGraphicModel.h"

#include "../Config.h"
#include "Vertex.h"
#include <algorithm>
#include <memory>

using namespace juce::gl;

#include "../Audio/UnitConverter.h"

// how many additional point are draw beyond the filter limit to display the gain reduction over next freqs
#define FILTERS_FADE_DEFINITION 4

// how many decibels per step we display when we draw the the samples filter fade out
#define FILTERS_FADE_STEP_DB 12.0f

SampleGraphicModel::SampleGraphicModel(std::shared_ptr<SamplePlayer> sp, juce::Colour col)
{
    reuseTexture = false;

    displayedSample = sp;

    if (sp == nullptr || !sp->hasBeenInitialized())
    {
        std::cerr << "Warning: trying to display unitialized sample !" << std::endl;
        return;
    }

    loaded = false;
    disabled = false;

    color = col;

    reloadSampleData(sp);

    // try to see if this texture already exists or not in texture manager
    auto optionalTextureId = textureManager->getTextureIdentifier(sp);
    reuseTexture = optionalTextureId.hasValue();
    if (reuseTexture)
    {
        tbo = *optionalTextureId;
        texture = textureManager->getTextureDataFromId(tbo);
        textureManager->declareTextureUsage(tbo);
    }

    // set a values related to fft data navigation
    std::shared_ptr<std::vector<float>> ffts = sp->getFftData();
    // the texture scaling in opengl use either manhatan distance or linear sum
    // of neigbouring pixels. So as we have less pixel over time than pixel over
    // frequencies, a glitch appear and the values leak to the sides.
    // This multiplier should help reduce horizontal leakage of textures.
    // If this is raised too high, this can blow up the GPU memory real fast.
    // If you change this value, make sure to stress test the loading a bit on shitty GPUs.
    horizontalScaleMultiplier = 4;
    numFfts = sp->getNumFft();
    numChannels = sp->getBufferNumChannels();
    textureHeight = 2 * FREQVIEW_SAMPLE_FFT_SCOPE_SIZE;
    textureWidth = numFfts * horizontalScaleMultiplier;
    channelTextureShift = textureWidth * (textureHeight >> 1) * 4;

    if (!reuseTexture)
    {
        // strangely enough, passing the initializer list is required otherwise the vector
        // object is broken
        texture = std::make_shared<std::vector<float>>();
        // reserve the size of the displayed texture
        texture->resize(textureHeight * textureWidth * 4); // 4 is for rgba values
        std::fill(texture->begin(), texture->end(), 1.0f);

        loadFftDataToTexture(ffts, numFfts, numChannels);
    }
}

void SampleGraphicModel::loadFftDataToTexture(std::shared_ptr<std::vector<float>> ffts, int fftCount, int channelCount)
{

    // NOTE: we store the texture colors (fft intensity) as RGBA.
    // This implies some harsh data duplication, but openGL
    // requires texture lines to be 4bytes aligned and they
    // end up compressed and in this format anyway down the line.
    // We won't put the color in as we will mix it later in the
    // opengl shader with the vertex color.

    // NOTE: according to
    // https://gamedev.stackexchange.com/questions/133849/row-order-in-opengl-dxt-texture
    // which seems to quote an outdated version of the OpenGL doc, texture
    // data start from bottom left corner and fill the row left to right, then
    // all the rows above up to the final top right corner.

    float intensity = 0.0f;

    int texturePos = 0;
    int freqiZoomed = 0;

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
                (*ffts)[(ffti * FREQVIEW_SAMPLE_FFT_SCOPE_SIZE) + (FREQVIEW_SAMPLE_FFT_SCOPE_SIZE - (freqiZoomed + 1))];
            // increase contrast and map between 0 and 1
            intensity = UnitConverter::magnifyIntensity(intensity);

            for (int nDuplicate = 0; nDuplicate < horizontalScaleMultiplier; nDuplicate++)
            {
                texturePos = getTextureIndex(freqi, ffti, nDuplicate, true);
                // now we write the intensity into the texture
                texture->data()[texturePos] = 1.0f;
                texture->data()[texturePos + 1] = 1.0f;
                texture->data()[texturePos + 2] = 1.0f;
                texture->data()[texturePos + 3] = intensity;
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
                intensity = (*ffts)[channelFftsShift + (ffti * FREQVIEW_SAMPLE_FFT_SCOPE_SIZE) + freqiZoomed];
            }
            else
            {
                intensity = (*ffts)[(ffti * FREQVIEW_SAMPLE_FFT_SCOPE_SIZE) + freqiZoomed];
            }
            intensity = UnitConverter::magnifyIntensity(intensity);

            for (int nDuplicate = 0; nDuplicate < horizontalScaleMultiplier; nDuplicate++)
            {
                texturePos = getTextureIndex(freqi, ffti, nDuplicate, false);
                texture->data()[texturePos] = 1.0f;
                texture->data()[texturePos + 1] = 1.0f;
                texture->data()[texturePos + 2] = 1.0f;
                texture->data()[texturePos + 3] = intensity;
            }
        }
    }
}

void SampleGraphicModel::reloadSampleData(std::shared_ptr<SamplePlayer> sp)
{

    // horizontal texture position data
    bufferStartPosRatio = float(sp->getBufferStart()) / float(sp->getTotalLength());
    bufferEndPosRatio = float(sp->getBufferEnd()) / float(sp->getTotalLength());
    bufferStartPosRatioAfterFadeIn = float(sp->getBufferStart() + sp->getFadeInLength()) / float(sp->getTotalLength());
    bufferEndPosRatioBeforeFadeOut = float(sp->getBufferEnd() - sp->getFadeOutLength()) / float(sp->getTotalLength());

    // horizontal vertice position data
    float leftX = float(sp->getEditingPosition());
    float rightX = leftX + float(sp->getLength());
    lastFadeInFrameLength = sp->getFadeInLength();
    lastFadeOutFrameLength = sp->getFadeOutLength();

    // vertical frequency positions
    lastLowPassFreq = sp->getLowPassFreq();
    lastHighPassFreq = sp->getHighPassFreq();

    updateFiltersGainReductionSteps(sp);

    // call the function that dynamically generates the vertices
    generateAndUploadVerticesToGPU(leftX, rightX, lastLowPassFreq, lastHighPassFreq, lastFadeInFrameLength,
                                   lastFadeOutFrameLength);
}

void SampleGraphicModel::updateFiltersGainReductionSteps(std::shared_ptr<SamplePlayer> sp)
{
    lowPassGainReductionSteps.reserve(FILTERS_FADE_DEFINITION);
    lowPassGainReductionSteps.clear();

    highPassGainReductionSteps.reserve(FILTERS_FADE_DEFINITION);
    highPassGainReductionSteps.clear();

    for (size_t i = 0; i < FILTERS_FADE_DEFINITION; i++)
    {
        lowPassGainReductionSteps.push_back(SamplePlayer::freqForFilterDbReduction(
            false, sp->getLowPassFreq(), (i + 1) * FILTERS_FADE_STEP_DB, sp->getLowPassRepeat()));
        highPassGainReductionSteps.push_back(SamplePlayer::freqForFilterDbReduction(
            true, sp->getHighPassFreq(), (i + 1) * FILTERS_FADE_STEP_DB, sp->getHighPassRepeat()));
    }
}

void SampleGraphicModel::generateAndUploadVerticesToGPU(float leftX, float rightX, float lowPassFreq,
                                                        float highPassFreq, float fadeInFrames, float fadeOutFrames)
{
    // how many vertice line there are vertically (sample fade start, sample start, sample end, sample fade end)
    int noVerticalVerticeLines = 4;
    // how many horizontal lines in the mesh (2 times cause there are symetrical bottom and top parts)
    int noHorizontalVerticeLines = 2 * (2 + (FILTERS_FADE_DEFINITION * 2));

    int noVertices = noVerticalVerticeLines * noHorizontalVerticeLines;
    int noSquares = (noVerticalVerticeLines - 1) * (noHorizontalVerticeLines - 1);
    int noTriangles = noSquares * 2;     // how many triangles we have to draw
    int noTriangleIds = noTriangles * 3; // how many ids we need to push to draw the triangles

    vertices.reserve(noVertices);
    triangleIds.reserve(noTriangleIds);

    vertices.clear();
    triangleIds.clear();

    for (size_t i = 0; i < noVertices; i++)
    {
        vertices.push_back({{0.0f, 0.0f},                                                             // position
                            {color.getFloatRed(), color.getFloatGreen(), color.getFloatBlue(), 1.0f}, // color
                            {0.0f, 0.0f}});                                                           // texture positon
    }

    // NOTE: We count vertices from left to right and top to bottom (like western text reading order)

    // set values of low pass filter fade out step lines
    for (int i = 0; i < FILTERS_FADE_DEFINITION; i++)
    {
        float glDistanceToCenter = UnitConverter::freqToPositionRatio(lowPassGainReductionSteps[i]);
        float textureDistanceToCenter = glDistanceToCenter * 0.5;
        float alphaLevel =
            juce::jlimit(0.0f, 1.0f, (float(FILTERS_FADE_DEFINITION - 1 - i) / float(FILTERS_FADE_DEFINITION)));

        setVerticeLineValue(TEXTURE_POSITION_Y, HORIZONTAL_VERTEX_LINE, FILTERS_FADE_DEFINITION - 1 - i,
                            0.5f + textureDistanceToCenter);
        setVerticeLineValue(TEXTURE_POSITION_Y, HORIZONTAL_VERTEX_LINE,
                            noHorizontalVerticeLines - FILTERS_FADE_DEFINITION + i, 0.5f - textureDistanceToCenter);

        setVerticeLineValue(VERTEX_POSITION_Y, HORIZONTAL_VERTEX_LINE, FILTERS_FADE_DEFINITION - 1 - i,
                            -glDistanceToCenter);
        setVerticeLineValue(VERTEX_POSITION_Y, HORIZONTAL_VERTEX_LINE,
                            noHorizontalVerticeLines - FILTERS_FADE_DEFINITION + i, glDistanceToCenter);

        setVerticeLineValue(VERTEX_ALPHA_LEVEL, HORIZONTAL_VERTEX_LINE, FILTERS_FADE_DEFINITION - 1 - i, alphaLevel);
        setVerticeLineValue(VERTEX_ALPHA_LEVEL, HORIZONTAL_VERTEX_LINE,
                            noHorizontalVerticeLines - FILTERS_FADE_DEFINITION + i, alphaLevel);
    }

    int lastTopPartIndex = (noHorizontalVerticeLines >> 1) - 1;

    // same for high pass filters
    for (int i = 0; i < FILTERS_FADE_DEFINITION; i++)
    {
        float glDistanceToCenter = UnitConverter::freqToPositionRatio(highPassGainReductionSteps[i]);
        float textureDistanceToCenter = glDistanceToCenter * 0.5;
        float alphaLevel =
            juce::jlimit(0.0f, 1.0f, (float(FILTERS_FADE_DEFINITION - 1 - i) / float(FILTERS_FADE_DEFINITION)));

        setVerticeLineValue(TEXTURE_POSITION_Y, HORIZONTAL_VERTEX_LINE,
                            lastTopPartIndex - FILTERS_FADE_DEFINITION + 1 + i, 0.5f + textureDistanceToCenter);
        setVerticeLineValue(TEXTURE_POSITION_Y, HORIZONTAL_VERTEX_LINE, lastTopPartIndex + 1 + i,
                            0.5f - textureDistanceToCenter);

        setVerticeLineValue(VERTEX_POSITION_Y, HORIZONTAL_VERTEX_LINE,
                            lastTopPartIndex - FILTERS_FADE_DEFINITION + 1 + i, -glDistanceToCenter);
        setVerticeLineValue(VERTEX_POSITION_Y, HORIZONTAL_VERTEX_LINE, lastTopPartIndex + 1 + i, glDistanceToCenter);

        setVerticeLineValue(VERTEX_ALPHA_LEVEL, HORIZONTAL_VERTEX_LINE,
                            lastTopPartIndex - FILTERS_FADE_DEFINITION + 1 + i, alphaLevel);
        setVerticeLineValue(VERTEX_ALPHA_LEVEL, HORIZONTAL_VERTEX_LINE, lastTopPartIndex + 1 + i, alphaLevel);
    }

    // set the two lines of the top and bottom part low pass filters frequencies
    float lowPassGlPosition = UnitConverter::freqToPositionRatio(lastLowPassFreq);
    setVerticeLineValue(VERTEX_POSITION_Y, HORIZONTAL_VERTEX_LINE, FILTERS_FADE_DEFINITION, -lowPassGlPosition);
    setVerticeLineValue(VERTEX_POSITION_Y, HORIZONTAL_VERTEX_LINE,
                        noHorizontalVerticeLines - FILTERS_FADE_DEFINITION - 1, lowPassGlPosition);
    float lowPassTexturePos = lowPassGlPosition / 2.0f;
    setVerticeLineValue(TEXTURE_POSITION_Y, HORIZONTAL_VERTEX_LINE, FILTERS_FADE_DEFINITION, 0.5f + lowPassTexturePos);
    setVerticeLineValue(TEXTURE_POSITION_Y, HORIZONTAL_VERTEX_LINE,
                        noHorizontalVerticeLines - FILTERS_FADE_DEFINITION - 1, 0.5f - lowPassTexturePos);

    // same for high pass
    float highPassGlPosition = UnitConverter::freqToPositionRatio(lastHighPassFreq);
    setVerticeLineValue(VERTEX_POSITION_Y, HORIZONTAL_VERTEX_LINE, lastTopPartIndex, -highPassGlPosition);
    setVerticeLineValue(VERTEX_POSITION_Y, HORIZONTAL_VERTEX_LINE, lastTopPartIndex + 1, highPassGlPosition);
    float highPassTexturePos = highPassGlPosition / 2.0f;
    setVerticeLineValue(TEXTURE_POSITION_Y, HORIZONTAL_VERTEX_LINE, lastTopPartIndex, 0.5f + highPassTexturePos);
    setVerticeLineValue(TEXTURE_POSITION_Y, HORIZONTAL_VERTEX_LINE, lastTopPartIndex + 1, 0.5f - highPassGlPosition);

    // now we set the parameters of the four vertical lines of vertices
    setVerticeLineValue(VERTEX_POSITION_X, VERTICAL_VERTEX_LINE, 0, leftX);
    setVerticeLineValue(VERTEX_POSITION_X, VERTICAL_VERTEX_LINE, 1, leftX + fadeInFrames);
    setVerticeLineValue(VERTEX_POSITION_X, VERTICAL_VERTEX_LINE, 2, rightX - fadeOutFrames);
    setVerticeLineValue(VERTEX_POSITION_X, VERTICAL_VERTEX_LINE, 3, rightX);

    setVerticeLineValue(TEXTURE_POSITION_X, VERTICAL_VERTEX_LINE, 0, bufferStartPosRatio);
    setVerticeLineValue(TEXTURE_POSITION_X, VERTICAL_VERTEX_LINE, 1, bufferStartPosRatioAfterFadeIn);
    setVerticeLineValue(TEXTURE_POSITION_X, VERTICAL_VERTEX_LINE, 2, bufferEndPosRatioBeforeFadeOut);
    setVerticeLineValue(TEXTURE_POSITION_X, VERTICAL_VERTEX_LINE, 3, bufferEndPosRatio);

    setVerticeLineValue(VERTEX_ALPHA_LEVEL, VERTICAL_VERTEX_LINE, 0, 0.0f);
    setVerticeLineValue(VERTEX_ALPHA_LEVEL, VERTICAL_VERTEX_LINE, 3, 0.0f);

    // we loop over all squares to connect them together (the square connection actually append trangle ids to be picked
    // by opengl)
    for (int i = 0; i < noHorizontalVerticeLines - 1; i++)
    {
        int horizontalLineTopLeftVerticeId = i * noHorizontalVerticeLines;
        int horizontalLineBottomLeftVerticeId = horizontalLineTopLeftVerticeId + noHorizontalVerticeLines;

        for (int j = 0; j < noVerticalVerticeLines - 1; j++)
        {
            connectSquareFromVertexIds(horizontalLineTopLeftVerticeId + j, horizontalLineTopLeftVerticeId + j + 1,
                                       horizontalLineBottomLeftVerticeId + j + 1,
                                       horizontalLineBottomLeftVerticeId + j);
        }
    }

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
    float leftX = vertices[8].position[0];
    float rightX = vertices[9].position[0];
    color = col;
    generateAndUploadVerticesToGPU(leftX, rightX, lastLowPassFreq, lastHighPassFreq, lastFadeInFrameLength,
                                   lastFadeOutFrameLength);
}

// Save position and width when selection dragging begins.
void SampleGraphicModel::initDrag()
{
    dragStartPosition = vertices[8].position[0];
    lastWidth = vertices[9].position[0] - vertices[8].position[0];
}

// To run on opengl thread, will update track position to account
// for the new shift during selection dragging.
void SampleGraphicModel::updateDrag(int frameMove)
{
    int position = dragStartPosition + frameMove;
    generateAndUploadVerticesToGPU(position, position + lastWidth, lastLowPassFreq, lastHighPassFreq,
                                   lastFadeInFrameLength, lastFadeOutFrameLength);
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

    float xInAudioBuffer = juce::jmap(x, bufferStartPosRatio, bufferEndPosRatio);
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

    float intensity = texture->data()[getTextureIndex(freqIndexNormalised, timeIndex, 1, y < 0.5) + 3];

    // now we apply the gain ramps if it falls in the
    if (xInAudioBuffer < bufferStartPosRatioAfterFadeIn)
    {
        intensity = intensity *
                    ((xInAudioBuffer - bufferStartPosRatio) / (bufferStartPosRatioAfterFadeIn - bufferStartPosRatio));
    }
    else if (xInAudioBuffer > bufferEndPosRatioBeforeFadeOut)
    {
        intensity = intensity * (1.0f - ((xInAudioBuffer - bufferEndPosRatioBeforeFadeOut) /
                                         (bufferEndPosRatio - bufferEndPosRatioBeforeFadeOut)));
    }

    return intensity;
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
    return juce::int64(vertices[8].position[0]);
}

juce::int64 SampleGraphicModel::getFrameLength()
{
    return juce::int64(vertices[9].position[0] - vertices[8].position[0]);
}

std::vector<juce::Rectangle<float>> SampleGraphicModel::getPixelBounds(float viewPosition, float viewScale,
                                                                       float viewHeight)
{
    float freqRatioLowPass = UnitConverter::freqToPositionRatio(lastLowPassFreq);
    float freqRatioHighPass = UnitConverter::freqToPositionRatio(lastHighPassFreq);
    float height = (freqRatioLowPass - freqRatioHighPass) * viewHeight;

    std::vector<juce::Rectangle<float>> rectangles;

    rectangles.push_back(juce::Rectangle<float>(
        (vertices[8].position[0] - viewPosition) / viewScale, (1.0 - freqRatioLowPass) * (viewHeight / 2.0),
        (vertices[9].position[0] - vertices[8].position[0]) / viewScale, height / 2.0));

    rectangles.push_back(juce::Rectangle<float>((vertices[8].position[0] - viewPosition) / viewScale,
                                                (viewHeight / 2.0) + ((freqRatioHighPass) * (viewHeight / 2.0)),
                                                (vertices[9].position[0] - vertices[8].position[0]) / viewScale,
                                                height / 2.0));

    return rectangles;
}
