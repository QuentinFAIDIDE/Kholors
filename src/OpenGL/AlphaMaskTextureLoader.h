#ifndef ALPHA_MASK_TEXTURE_LOADER_HPP
#define ALPHA_MASK_TEXTURE_LOADER_HPP

#include "../Config.h"
#include "juce_opengl/opengl/juce_gl.h"
#include <juce_opengl/juce_opengl.h>

using namespace juce::gl;

/**
 Class responsible for loading the default sample alpha mark
 that is mixed with the displayed fft result.
 Note that we didn't rely on juce's code, mostly because we want to micromanage
 texture compression, mipmaps and layers of textures as it's
 a critical bottleneck (for gpu with few memory).
 */
class AlphaMaskTextureLoader
{
  public:
    AlphaMaskTextureLoader(){};
    ~AlphaMaskTextureLoader(){};
    void loadTexture()
    {
        juce::String filepath = FREQVIEW_SAMPLE_MASK_PATH;
        juce::File maskFile(filepath);

        if (maskFile.existsAsFile())
        {
            loadFileRgbaTexture(maskFile);
        }
        else
        {
            loadBlankRgbaTexture();
        }

        createOpenGlTexture();
    };

    void bindTexture()
    {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, tbo);
    }

  private:
    std::vector<float> textureData;
    int width, height;
    GLuint tbo;

    void loadBlankRgbaTexture()
    {
        width = 100;
        height = 100;

        textureData.resize(width * height * 4);

        for (int i = 0; i < (width * height); i++)
        {
            int texturePos = i * 4;
            textureData[texturePos] = 1.0f;
            textureData[texturePos + 1] = 1.0f;
            textureData[texturePos + 2] = 1.0f;
            textureData[texturePos + 3] = 1.0f;
        }
    }

    void loadFileRgbaTexture(juce::File &file)
    {
        juce::Image alphaMask;
        alphaMask = juce::ImageFileFormat::loadFrom(file);
        if (!alphaMask.isValid())
        {
            std::cerr << "Invalid provided sample alpha mask image." << std::endl;
            loadBlankRgbaTexture();
            return;
        }

        width = alphaMask.getWidth();
        height = alphaMask.getHeight();

        textureData.resize(width * height * 4);

        juce::Colour pixelIntensity;

        for (int i = 0; i < width; i++)
        {
            for (int j = 0; j < height; j++)
            {

                pixelIntensity = alphaMask.getPixelAt(i, j);

                int texturePos = ((i * height) + j) * 4;

                // we only need one component (let's pick red to support non alpha imgs)
                textureData[texturePos] = pixelIntensity.getFloatRed();
                textureData[texturePos + 1] = pixelIntensity.getFloatRed();
                textureData[texturePos + 2] = pixelIntensity.getFloatRed();
                textureData[texturePos + 3] = pixelIntensity.getFloatRed();
            }
        }
    }

    void createOpenGlTexture()
    {
        // register the texture
        glGenTextures(1, &tbo);
        glBindTexture(GL_TEXTURE_2D, tbo);
        // set the texture wrapping
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // set the filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // send the texture to the gpu
        glTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, textureData.data());

        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR)
        {
            std::cerr << "got following open gl error after uploading alpha mask texture data: " << err << std::endl;
        }
    }
};

#endif // ALPHA_MASK_TEXTURE_LOADER_HPP