#include "SampleGraphicModel.h"

#include "../Config.h"
#include "Vertex.h"
#include <algorithm>
#include <memory>
#include <stdexcept>

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
        loaded = false;
        disabled = true;
        // this happens when we load a project and we want to load
        // an empty sample so that the ids match in mixbus and graphic
        // models.
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
    horizontalScaleMultiplier = 3;

    numFfts = sp->getNumFft();
    numChannels = sp->getBufferNumChannels();
    textureHeight = 2 * FFT_STORAGE_SCOPE_SIZE;
    textureWidth = numFfts * horizontalScaleMultiplier;
    channelTextureShift = textureWidth * (textureHeight >> 1) * 4;

    if (!reuseTexture)
    {
        // strangely enough, passing the initializer list is required otherwise the vector
        // object is broken
        texture = std::make_shared<std::vector<float>>();
        // reserve the size of the displayed texture
        texture->resize((size_t)textureHeight * (size_t)textureWidth * 4); // 4 is for rgba values
        std::fill(texture->begin(), texture->end(), 1.0f);

        loadFftDataToTexture(ffts);
    }
}

void SampleGraphicModel::loadFftDataToTexture(std::shared_ptr<std::vector<float>> ffts)
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

    int channelFftsShift = numFfts * FFT_STORAGE_SCOPE_SIZE;

    // for each fourier transform over time
    for (int ffti = 0; ffti < numFfts; ffti++)
    {
        // for each frequency of the texture (linear to displayed texture)
        for (int freqi = 0; freqi < FFT_STORAGE_SCOPE_SIZE; freqi++)
        {
            // we apply our polynomial lens freqi transformation to zoom in a bit
            freqiZoomed = UnitConverter::magnifyTextureFrequencyIndex(freqi);
            // as the frequencies in the ffts goes from low to high, we have
            // to flip the freqi to fetch the frequency and it's all good !
            intensity =
                (*ffts)[((size_t)ffti * FFT_STORAGE_SCOPE_SIZE) + (FFT_STORAGE_SCOPE_SIZE - ((size_t)freqiZoomed + 1))];
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
            freqiZoomed = FFT_STORAGE_SCOPE_SIZE -
                          (UnitConverter::magnifyTextureFrequencyIndex((FFT_STORAGE_SCOPE_SIZE - (freqi + 1))) + 1);
            // get the value depending on if we got a second channel or not
            if (numChannels == 2)
            {
                intensity = (*ffts)[(size_t)(channelFftsShift + (ffti * FFT_STORAGE_SCOPE_SIZE) + freqiZoomed)];
            }
            else
            {
                intensity = (*ffts)[(size_t)((ffti * FFT_STORAGE_SCOPE_SIZE) + freqiZoomed)];
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
    generateAndUploadVerticesToGPU(leftX, rightX, lastFadeInFrameLength, lastFadeOutFrameLength);
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

void SampleGraphicModel::generateAndUploadVerticesToGPU(float leftX, float rightX, float fadeInFrames,
                                                        float fadeOutFrames)
{
    // how many vertice line there are vertically (sample fade start, sample start, sample end, sample fade end)
    noVerticalVerticeLines = 4;
    // how many horizontal lines in the mesh (2 times cause there are symetrical bottom and top parts)
    noHorizontalVerticeLines = 2 * (2 + (FILTERS_FADE_DEFINITION * 2));

    int noVertices = noVerticalVerticeLines * noHorizontalVerticeLines;
    int noSquares = (noVerticalVerticeLines - 1) * (noHorizontalVerticeLines - 1);
    int noTriangles = noSquares * 2;     // how many triangles we have to draw
    int noTriangleIds = noTriangles * 3; // how many ids we need to push to draw the triangles

    vertices.reserve((size_t)noVertices);
    triangleIds.reserve((size_t)noTriangleIds);

    vertices.clear();
    triangleIds.clear();

    for (size_t i = 0; i < (size_t)noVertices; i++)
    {
        vertices.push_back({{0.0f, 0.0f},                                                             // position
                            {color.getFloatRed(), color.getFloatGreen(), color.getFloatBlue(), 1.0f}, // color
                            {0.0f, 0.0f}});                                                           // texture positon
    }

    // NOTE: We count vertices from left to right and top to bottom (like western text reading order)

    // set values of low pass filter fade out step lines
    for (int i = 0; i < FILTERS_FADE_DEFINITION; i++)
    {
        float glDistanceToCenter = UnitConverter::freqToPositionRatio(lowPassGainReductionSteps[(size_t)i]);
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
        float glDistanceToCenter = UnitConverter::freqToPositionRatio(highPassGainReductionSteps[(size_t)i]);
        float textureDistanceToCenter = glDistanceToCenter * 0.5;
        float alphaLevel =
            juce::jlimit(0.0f, 1.0f, (float(FILTERS_FADE_DEFINITION - 1 - i) / float(FILTERS_FADE_DEFINITION)));

        setVerticeLineValue(TEXTURE_POSITION_Y, HORIZONTAL_VERTEX_LINE,
                            lastTopPartIndex - FILTERS_FADE_DEFINITION + 1 + i, 0.5f + textureDistanceToCenter);
        setVerticeLineValue(TEXTURE_POSITION_Y, HORIZONTAL_VERTEX_LINE, lastTopPartIndex + FILTERS_FADE_DEFINITION - i,
                            0.5f - textureDistanceToCenter);

        setVerticeLineValue(VERTEX_POSITION_Y, HORIZONTAL_VERTEX_LINE,
                            lastTopPartIndex - FILTERS_FADE_DEFINITION + 1 + i, -glDistanceToCenter);
        setVerticeLineValue(VERTEX_POSITION_Y, HORIZONTAL_VERTEX_LINE, lastTopPartIndex + FILTERS_FADE_DEFINITION - i,
                            glDistanceToCenter);

        setVerticeLineValue(VERTEX_ALPHA_LEVEL, HORIZONTAL_VERTEX_LINE,
                            lastTopPartIndex - FILTERS_FADE_DEFINITION + 1 + i, alphaLevel);
        setVerticeLineValue(VERTEX_ALPHA_LEVEL, HORIZONTAL_VERTEX_LINE, lastTopPartIndex + FILTERS_FADE_DEFINITION - i,
                            alphaLevel);
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
    int topHighPassLineIndex = lastTopPartIndex - FILTERS_FADE_DEFINITION;
    int botHighPassLineIndex = lastTopPartIndex + FILTERS_FADE_DEFINITION + 1;
    setVerticeLineValue(VERTEX_POSITION_Y, HORIZONTAL_VERTEX_LINE, topHighPassLineIndex, -highPassGlPosition);
    setVerticeLineValue(VERTEX_POSITION_Y, HORIZONTAL_VERTEX_LINE, botHighPassLineIndex, highPassGlPosition);
    float highPassTexturePos = highPassGlPosition / 2.0f;
    setVerticeLineValue(TEXTURE_POSITION_Y, HORIZONTAL_VERTEX_LINE, topHighPassLineIndex, 0.5f + highPassTexturePos);
    setVerticeLineValue(TEXTURE_POSITION_Y, HORIZONTAL_VERTEX_LINE, botHighPassLineIndex, 0.5f - highPassTexturePos);

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
        // we do not connect the top and bottom part (left and right audio channel drawing)
        if (i == (noHorizontalVerticeLines >> 1) - 1)
        {
            continue;
        }

        int horizontalLineTopLeftVerticeId = i * noVerticalVerticeLines;
        int horizontalLineBottomLeftVerticeId = horizontalLineTopLeftVerticeId + noVerticalVerticeLines;

        for (int j = 0; j < noVerticalVerticeLines - 1; j++)
        {
            connectSquareFromVertexIds(
                (size_t)(horizontalLineTopLeftVerticeId + j), (size_t)(horizontalLineTopLeftVerticeId + j + 1),
                (size_t)(horizontalLineBottomLeftVerticeId + j + 1), (size_t)(horizontalLineBottomLeftVerticeId + j));
        }
    }

    uploadVerticesToGpu();
}

Vertex &SampleGraphicModel::getUpperLeftCorner()
{
    return vertices[FILTERS_FADE_DEFINITION * (size_t)noVerticalVerticeLines];
}

Vertex &SampleGraphicModel::getUpperRightCorner()
{
    return vertices[((FILTERS_FADE_DEFINITION + 1) * (size_t)noVerticalVerticeLines) - 1];
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
    float leftX = getUpperLeftCorner().position[0];
    float rightX = getUpperRightCorner().position[0];
    color = col;
    generateAndUploadVerticesToGPU(leftX, rightX, lastFadeInFrameLength, lastFadeOutFrameLength);
}

// Save position and width when selection dragging begins.
void SampleGraphicModel::initDrag()
{
    dragStartPosition = getUpperLeftCorner().position[0];
    lastWidth = getUpperRightCorner().position[0] - getUpperLeftCorner().position[0];
}

// To run on opengl thread, will update track position to account
// for the new shift during selection dragging.
void SampleGraphicModel::updateDrag(int frameMove)
{
    int position = dragStartPosition + frameMove;
    generateAndUploadVerticesToGPU(position, position + lastWidth, lastFadeInFrameLength, lastFadeOutFrameLength);
}

void SampleGraphicModel::uploadVerticesToGpu()
{
    const juce::ScopedLock lock(loadingMutex);

    glBindVertexArray(vao);

    // register and upload the vertices data
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, (long)(sizeof(Vertex) * vertices.size()), vertices.data(), GL_STATIC_DRAW);
    // register and upload indices of the vertices to form the triangles
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (long)(sizeof(unsigned int) * triangleIds.size()), triangleIds.data(),
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

    if (isFullyFilteredArea(y))
    {
        return 0.0f;
    }

    float xInAudioBuffer = juce::jmap(x, bufferStartPosRatio, bufferEndPosRatio);
    int timeIndex = xInAudioBuffer * numFfts;
    // index of zoomed frequencies, not linear to logarithm of frequencies
    int freqIndexNormalised = 0;
    if (y < 0.5)
    {
        freqIndexNormalised = y * 2 * FFT_STORAGE_SCOPE_SIZE;
    }
    else
    {
        freqIndexNormalised = (y - 0.5) * 2 * FFT_STORAGE_SCOPE_SIZE;
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

    int filteringLevelAtClick = getFilteringLevel(y);
    float intensityMultiplier = (1.0f - (float(filteringLevelAtClick) / (FILTERS_FADE_DEFINITION + 1)));
    intensity = intensity * intensityMultiplier;

    return intensity;
}

int SampleGraphicModel::isFullyFilteredArea(float y)
{
    return getFilteringLevel(y) == FILTERS_FADE_DEFINITION + 1;
}

int SampleGraphicModel::getFilteringLevel(float yPosition)
{
    // the position inside the channel (as opposed from the yPosition view that covers both channels)
    float yChannelPosition;

    if (yPosition < 0.5f)
    {
        yChannelPosition = 1.0f - (yPosition * 2.0f);
    }
    else
    {
        yChannelPosition = 2.0f * (yPosition - 0.5f);
    }

    // find out if inside the inner area
    if (yChannelPosition >= UnitConverter::freqToPositionRatio(lastHighPassFreq) &&
        yChannelPosition <= UnitConverter::freqToPositionRatio(lastLowPassFreq))
    {
        return 0;
    }

    // find out if inside one of the high pass fade out steps area
    float lastHighPassFreqStep = highPassGainReductionSteps[FILTERS_FADE_DEFINITION - 1];
    if (yChannelPosition >= UnitConverter::freqToPositionRatio(lastHighPassFreqStep) &&
        yChannelPosition <= UnitConverter::freqToPositionRatio(lastHighPassFreq))
    {
        for (int i = 0; i < FILTERS_FADE_DEFINITION - 1; i++)
        {
            float lowestBandFreqPos = UnitConverter::freqToPositionRatio(highPassGainReductionSteps[(size_t)i + 1]);
            float highestBandFreqPos = UnitConverter::freqToPositionRatio(highPassGainReductionSteps[(size_t)i]);

            if (yChannelPosition >= lowestBandFreqPos && yChannelPosition <= highestBandFreqPos)
            {
                return 2 + i;
            }
        }
        // if we found no match in the inner bands, it means the value is between the lowest step and the filter freq
        return 1;
    }

    // find out if inside one if the low pass fade out steps area
    float lastLowPassFreqStep = lowPassGainReductionSteps[FILTERS_FADE_DEFINITION - 1];
    if (yChannelPosition >= UnitConverter::freqToPositionRatio(lastLowPassFreq) &&
        yChannelPosition <= UnitConverter::freqToPositionRatio(lastLowPassFreqStep))
    {
        for (int i = 0; i < FILTERS_FADE_DEFINITION - 1; i++)
        {
            float lowestBandFreqPos = UnitConverter::freqToPositionRatio(lowPassGainReductionSteps[(size_t)i]);
            float highestBandFreqPos = UnitConverter::freqToPositionRatio(lowPassGainReductionSteps[(size_t)i + 1]);

            if (yChannelPosition >= lowestBandFreqPos && yChannelPosition <= highestBandFreqPos)
            {
                return 2 + i;
            }
        }
        // if we found no match in the inner bands, it means the value is between the lowest step and the filter freq
        return 1;
    }

    // if none of the above matched, we're in the filtered out area
    return FILTERS_FADE_DEFINITION + 1;
}

juce::int64 SampleGraphicModel::getFramePosition()
{
    return juce::int64(getUpperLeftCorner().position[0]);
}

juce::int64 SampleGraphicModel::getFrameLength()
{
    return juce::int64(getUpperRightCorner().position[0] - getUpperLeftCorner().position[0]);
}

std::vector<juce::Rectangle<float>> SampleGraphicModel::getPixelBounds(float viewPosition, float viewScale,
                                                                       float viewHeight)
{
    float freqRatioLowPass = UnitConverter::freqToPositionRatio(lastLowPassFreq);
    float freqRatioHighPass = UnitConverter::freqToPositionRatio(lastHighPassFreq);
    float height = (freqRatioLowPass - freqRatioHighPass) * viewHeight;

    std::vector<juce::Rectangle<float>> rectangles;

    rectangles.push_back(juce::Rectangle<float>(
        (getUpperLeftCorner().position[0] - viewPosition) / viewScale, (1.0 - freqRatioLowPass) * (viewHeight / 2.0),
        (getUpperRightCorner().position[0] - getUpperLeftCorner().position[0]) / viewScale, height / 2.0));

    rectangles.push_back(juce::Rectangle<float>(
        (getUpperLeftCorner().position[0] - viewPosition) / viewScale,
        (viewHeight / 2.0) + ((freqRatioHighPass) * (viewHeight / 2.0)),
        (getUpperRightCorner().position[0] - getUpperLeftCorner().position[0]) / viewScale, height / 2.0));

    return rectangles;
}

void SampleGraphicModel::setVertexProperty(Vertex &vertexToChange, VerticeProperty propertyToChange, float value)
{
    switch (propertyToChange)
    {
    case TEXTURE_POSITION_X:
        vertexToChange.texturePosition[0] = value;
        break;
    case TEXTURE_POSITION_Y:
        vertexToChange.texturePosition[1] = value;
        break;
    case VERTEX_POSITION_X:
        vertexToChange.position[0] = value;
        break;
    case VERTEX_POSITION_Y:
        vertexToChange.position[1] = value;
        break;
    case VERTEX_ALPHA_LEVEL:
        vertexToChange.colour[3] = value;
        break;
    default:
        throw new std::runtime_error("Calling setVertexProperty with undefined VerticeProperty parameter");
    };
}

void SampleGraphicModel::setVerticeLineValue(VerticeProperty targetPropertyToSet, VerticeLineType lineType,
                                             int lineIndex, float value)
{
    if (lineType == HORIZONTAL_VERTEX_LINE)
    {
        int firstLineIndex = lineIndex * noVerticalVerticeLines;
        for (size_t i = 0; i < (size_t)noVerticalVerticeLines; i++)
        {
            setVertexProperty(vertices[(size_t)firstLineIndex + i], targetPropertyToSet, value);
        }
    }
    else if (lineType == VERTICAL_VERTEX_LINE)
    {
        int firstColumnIndex = lineIndex;
        for (size_t i = 0; i < (size_t)noHorizontalVerticeLines; i++)
        {
            setVertexProperty(vertices[(size_t)firstColumnIndex + (i * (size_t)noVerticalVerticeLines)],
                              targetPropertyToSet, value);
        }
    }
}