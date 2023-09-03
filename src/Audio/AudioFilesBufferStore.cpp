#include "AudioFilesBufferStore.h"

AudioFilesBufferStore::AudioFilesBufferStore()
{
}

std::shared_ptr<juce::AudioSampleBuffer> AudioFilesBufferStore::loadSample(std::string fullFilePath)
{
    // get the full file path on disk
    // check cache and return cached item if present
    // if not present, load it
}

void AudioFilesBufferStore::releaseUnusedBuffers()
{
}

void AudioFilesBufferStore::disableUnusedBuffersRelease()
{
}

void AudioFilesBufferStore::enableUnusedBuffersRelease()
{
}