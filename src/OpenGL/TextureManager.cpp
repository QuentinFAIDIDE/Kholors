#include "TextureManager.h"

juce::Optional<GLint> TextureManager::getTextureIdentifier(std::shared_ptr<SamplePlayer> sp)
{
    BufferPtr buffer = sp->getBufferRef();

    // if length is unique, directly return nothing
    if (lengthCounts.find(buffer->getAudioSampleBuffer()->getNumSamples()) == lengthCounts.end())
    {
        return juce::Optional<GLint>();
    }

    // compute hash
    int hashingLength = juce::jmin(buffer->getAudioSampleBuffer()->getNumSamples(), TEXTURE_MANAGER_HASH_LENGTH);
    size_t hash = hashAudioChannel(buffer->getAudioSampleBuffer()->getReadPointer(0), hashingLength);

    // if no items for this hash, return nothing
    auto foundHash = texturesPerHash.find(hash);
    if (foundHash == texturesPerHash.end())
    {
        return juce::Optional<GLint>();
    }

    std::vector<GLint> audioBufferBucket = foundHash->second;

    // for each hash item iterate and test full equality
    for (size_t i = 0; i < audioBufferBucket.size(); i++)
    {
        juce::Optional<BufferPtr> bucketAudioBuffer = getAudioBufferFromTextureIndex(textu);
        if (areAudioBufferEqual(buffer->getAudioSampleBuffer(), ))
    }
}

bool TextureManager::areAudioBufferEqual(juce::AudioBuffer<float> &a, juce::AudioBuffer<float> &b)
{
    if (a.getNumChannels() != b.getNumChannels())
    {
        return false;
    }

    if (a.getNumSamples() != b.getNumSamples())
    {
        return false;
    }

    for (size_t chan = 0; chan < a.getNumChannels(); chan++)
    {
        auto aData = a.getReadPointer(chan);
        auto bData = b.getReadPointer(chan);

        for (size_t sample = 0; sample < a.getNumSamples(); sample++)
        {
            if (aData[sample] != bData[sample])
            {
                return false;
            }
        }
    }

    return true;
}

BufferPtr TextureManager::getAudioBufferFromTextureIndex(GLint id)
{
}

size_t TextureManager::hashAudioChannel(const float *data, int length)
{
    // should not possibly happen
    if (length == 0)
    {
        return 0;
    }

    std::hash<float> hasher;

    if (length == 1)
    {
        return hasher(data[0]);
    }

    // note: I'm not sure hashing a float makes sense, as in some (most)
    // implementations size_t would be the same size as a float.

    // inspired by boost implementation of combined hash,
    // stolen from stack overflow here: https://stackoverflow.com/questions/2590677/how-do-i-combine-hash-values-in-c0x
    // lots of discussion detailing what it does and why on Stackoverflow
    size_t seed = 0;
    for (size_t i = 0; i < length; i++)
    {
        seed ^= hasher(data[i]) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    return seed;
}

void TextureManager::decrementUsageCount(GLint id)
{
    // if this GLint is in the count tables
    // decrement it
    // if count is zero, free this texture resources
}
