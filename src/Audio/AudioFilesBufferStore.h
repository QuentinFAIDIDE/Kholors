#ifndef DEF_AUDIO_FILES_BUFFER_STORE_HPP
#define DEF_AUDIO_FILES_BUFFER_STORE_HPP

#include <juce_audio_basics/juce_audio_basics.h>
#include <memory>
#include <vector>

struct AudioFileBufferRef
{
    std::shared_ptr<juce::AudioSampleBuffer> data;
};

class AudioFilesBufferStore
{
  public:
    /**
     * @brief      Constructs a new instance.
     */
    AudioFilesBufferStore();

    /**
     * @brief      Loads sample audio buffer at that path..
     *
     * @param[in]  fullFilePath  The full file path on disk.
     *
     * @return     A shared pointer to the audio buffer.
     */
    std::shared_ptr<juce::AudioSampleBuffer> loadSample(std::string fullFilePath);

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
    std::map<std::string, std::shared_ptr<juce::AudioSampleBuffer>>
        audioBuffersReferences; /**< map of full disk paths (without double slashes) to audio buffers */
};

#endif // DEF_AUDIO_FILES_BUFFER_STORE_HPP