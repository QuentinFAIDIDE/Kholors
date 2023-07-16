#ifndef DEF_TEXTURE_MANAGER_HPP
#define DEF_TEXTURE_MANAGER_HPP

#include "../Audio/SamplePlayer.h"
#include "juce_opengl/opengl/juce_gl.h"
#include <memory>

#define TEXTURE_MANAGER_HASH_LENGTH 1024

struct AudioBufferTextureData
{
    std::shared_ptr<std::vector<float>> textureData;
    BufferPtr audioData;
    int useCount;
};

class TextureManager
{
  public:
    /**
     * @brief      Gets a texture identifier if this audio buffer
     *             already has been converted to a texture. Return nothing
     *             if the buffer was never seen. If found, increment texture
     *             users count.
     *
     * @param[in]  sp   reference to the sample player object that holds the audio buffer.
     *
     * @return     The texture identifier.
     */
    juce::Optional<GLuint> getTextureIdentifier(std::shared_ptr<SamplePlayer> sp);

    /**
     * @brief      After the textured object was deleted, we ensure its texture
     *             usage count is decremented so that it can be freed whenever reaching
     *             zero. When usage count is lowered to zero, true is returned as to
     *             signal the caller needs to free the texture resource. If this function
     *             returns false, the texture should not be freed.
     *
     * @param[in]  id    the identifier of the texture that the object had.
     *
     * @return     True if the texture should be freed, false of not.
     */
    bool decrementUsageCount(GLuint);

    /**
     * @brief      Gets the audio buffer corresponding the texture index.
     *
     * @param[in]  id    The identifier of the texture
     *
     * @return     The audio buffer (counted) reference.
     */
    BufferPtr getAudioBufferFromTextureId(GLuint id);

    /**
     * @brief      Gets the texture data from identifier.
     *
     * @param[in]  id the open gl id that was return by getTextureIdentifier or passed to setTextureIdentifier
     *
     * @return     The texture data from identifier.
     */
    std::shared_ptr<std::vector<float>> getTextureDataFromId(GLuint);

    /**
     * @brief      Te be called when a texture was registered to openGL, will save the texture
     *             identifier to share it, as well as raw texture data and sample audio for
     *             identification purpose of forthcoming duplicated that needs to reuse the texture.
     *
     * @param[in]  index        The index
     * @param[in]  textureData  The texture data
     * @param[in]  sp           The new value
     */
    void setTexture(GLuint index, std::shared_ptr<SamplePlayer> sp);

    /**
     * @brief      Declares that a texture id was used in a new opengl object.
     *             This increments usage count.
     */
    void declareTextureUsage(GLuint textureId);

  private:
    /**
     * @brief      free the texture resources for this texture id.
     *
     * @param[in]  id    The identifier
     */
    void freeTextureIdResources(GLuint id);

    /**
     * @brief      Generate a hash of the data provided up to length.
     *
     * @param[in]  data    The data
     * @param[in]  length  The length
     *
     * @return     a hash of the first length samples of the floats at data
     */
    size_t hashAudioChannel(const float *data, int length);

    /**
     * @brief      test if two audio buffer are equal on all channels
     *
     * @return     true if equal, false if not
     */
    bool areAudioBufferEqual(juce::AudioBuffer<float> &, juce::AudioBuffer<float> &);

    /**
     * @brief      Clear all data for this texture id. To be called after
     *             the count of texture usage fell to zero.
     *
     * @param[in]  textureId  The texture identifier
     */
    void clearTextureData(GLuint textureId);

    // tells how many textures audio have that length. Used to quickly rule out
    // that some sample is already stored here.
    std::map<int, int> texturesLengthCount;

    // bucket of textures based on their audio left channel hash
    std::map<size_t, std::vector<GLuint>> texturesPerHash;

    // map from GLuint texture ids to audio buffer and texture data
    std::map<GLuint, std::shared_ptr<AudioBufferTextureData>> audioBufferTextureData;
};

#endif // DEF_TEXTURE_MANAGER_HPP