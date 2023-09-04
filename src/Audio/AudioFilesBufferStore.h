#ifndef DEF_AUDIO_FILES_BUFFER_STORE_HPP
#define DEF_AUDIO_FILES_BUFFER_STORE_HPP

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <memory>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <stdexcept>
#include <vector>

/**
 * @brief A structure holding pointer to audio data along its
          file on disk and a SHA1 digest.
 *
 */
struct AudioFileBufferRef
{
    /**
     * @brief Construct a new Audio File Buffer Ref object
     *
     * @param ptr A pointer to an already allocated and read audio buffer for the file
     * @param path  The full file path on disk
     */
    AudioFileBufferRef(std::shared_ptr<juce::AudioSampleBuffer> ptr, std::string path);

    /**
     * @brief Return a hexadecimal string representing the audio buffer hash.
     *
     * @return std::string
     */
    std::string hashDigest();

    std::shared_ptr<juce::AudioSampleBuffer> data; /**< pointer to the data */
    std::string fileFullPath;                      /**< full path to the file on disk */
    unsigned char hash[SHA_DIGEST_LENGTH];         /**< hash of the audio content (used for fallback object storage) */
};

/**
 * @brief Describes a class that is storing audio buffer for files
 *        and cache them based on file path. It has a callback to
 *        free the buffer that are not referenced anymore outside
 *        of the store, and this behaviour can be paused and resumed
 *        for whenever the user wants to reload a project and have the
 *        the samples potentially reused.
 *
 */
class AudioFilesBufferStore
{
  public:
    AudioFilesBufferStore();

    /**
     * @brief      Loads sample audio buffer at that path..
     *
     * @param[in]  fullFilePath  The full file path on disk.
     *
     * @return     A structure with file info and shared pointer to audio data.
     */
    AudioFileBufferRef loadSample(std::string fullFilePath);

    /**
     * @brief      Free audio buffers that are not referenced anymore
     *             outside of this store.
     */
    void releaseUnusedBuffers();

    /**
     * @brief      Prevent freeing unused audio buffers with releaseUnusedBuffers untill
     *             enableUnusedBuffersRelease is called.
     *             Typically used when loading a new project to reuse sample buffers.
     *             It is important because the same project can be re-loaded if the
     *             user want to do a project hard reset or checkout another project commit
     *             and we don't want to waste time loading the same buffers again.
     */
    void disableUnusedBuffersRelease();

    /**
     * @brief      Allow unused samples to be cleared as normal by releaseUnusedBuffers
     *             after a call to disableUnusedBuffersRelease was made to pause that behaviour.
     *             Note that unused beffer release is enabled by default and this is useless
     *             to call this at startup if no disableUnusedBuffersRelease calls were made.
     */
    void enableUnusedBuffersRelease();

  private:
    juce::AudioFormatManager formatManager;
    bool allowUnusedBufferRelease;
    juce::CriticalSection lock;

    std::map<std::string, AudioFileBufferRef> audioBuffersCache; /**< map of full disk paths to audio buffers */
};

#endif // DEF_AUDIO_FILES_BUFFER_STORE_HPP