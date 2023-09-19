#ifndef DEF_TEXTURE_MANAGER_HPP
#define DEF_TEXTURE_MANAGER_HPP

#include "../Audio/SamplePlayer.h"
#include "juce_opengl/opengl/juce_gl.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_opengl/juce_opengl.h>
#include <memory>

#define TEXTURE_MANAGER_HASH_LENGTH 1024

struct AudioBufferTextureData
{
    std::shared_ptr<std::vector<float>> textureData;
    AudioFileBufferRef audioData;
    int useCount;
};

/**
 * @brief An object responsible for storing allocated texture data
 *        counting their users objects and eventually freeing the texture
 *        if the count reaches zero. It serves as a cache for avoiding costly
 *        re-allocations. Note that while it doesn't load the texture against openGL
 *        and expect the GLuint to be allocated, it clear them in OpenGL, which no
 *        textured object should ever do.
 *
 */
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
     * @brief      Decrement count of usage of this texture id and eventually frees it.
     *
     * @param[in]  id    the identifier of the texture that the object had.
     *
     */
    void decrementUsageCount(GLuint);

    /**
     * @brief      Gets the audio buffer corresponding the texture index.
     *
     * @param[in]  id    The identifier of the texture
     *
     * @return     The audio buffer (counted) reference.
     */
    AudioFileBufferRef getAudioBufferFromTextureId(GLuint id);

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
     * @param[in]  index        Texture index assigned by openGL
     * @param[in]  sp           Pointer for the displayed SamplePlayer
     * @param[in]  textureData  The texture data (RGBA one that is sent to GPU)
     */
    void setTexture(GLuint index, std::shared_ptr<SamplePlayer> sp, std::shared_ptr<std::vector<float>> textureData);

    /**
     * @brief      Declares that a texture id was used in a new opengl object.
     *             This increments usage count.
     */
    void declareTextureUsage(GLuint textureId);

    /**
     * @brief Increment count of all textures and save em so the count can be later
     * decrease with releaseLockedTextures.
     *
     */
    void lockCurrentTextures();

    /**
     * @brief If some texture have been locked, decrement their usage count and
     * eventually have them freed.
     *
     */
    void releaseLockedTextures();

    /**
     * @brief Set the Open Gl Context object
     *
     */
    void setOpenGlContext(juce::OpenGLContext *);

    /**
     * @brief tests if the texture is stored in the texture manager.
     *
     * @param textureId OpenGL texture identifier
     * @return true if the texture is cached in texture maanger
     * @return false if the texture is not cached in texture manager
     */
    bool textureIdIsStored(GLuint textureId);

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

    std::set<GLuint> lockedTextures; /**< texture for which count was incremented (eg "locked") */

    // bucket of textures based on their audio left channel hash
    std::map<size_t, std::vector<GLuint>> texturesPerHash;

    std::optional<juce::OpenGLContext *> glContext; /**< A reference to the opengl context where to free textures*/

    // map from GLuint texture ids to audio buffer and texture data
    std::map<GLuint, std::shared_ptr<AudioBufferTextureData>> audioBufferTextureData;
};

#endif // DEF_TEXTURE_MANAGER_HPP