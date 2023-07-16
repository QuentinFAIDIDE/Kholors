#include "TextureManager.h"
#include <memory>

juce::Optional<GLuint> TextureManager::getTextureIdentifier(std::shared_ptr<SamplePlayer> sp)
{
    BufferPtr buffer = sp->getBufferRef();

    // if length is unique, directly return nothing
    if (texturesLengthCount.find(buffer->getAudioSampleBuffer()->getNumSamples()) == texturesLengthCount.end())
    {
        return juce::Optional<GLuint>();
    }

    // compute hash
    int hashingLength = juce::jmin(buffer->getAudioSampleBuffer()->getNumSamples(), TEXTURE_MANAGER_HASH_LENGTH);
    size_t hash = hashAudioChannel(buffer->getAudioSampleBuffer()->getReadPointer(0), hashingLength);

    // if no items for this hash, return nothing
    auto foundHash = texturesPerHash.find(hash);
    if (foundHash == texturesPerHash.end())
    {
        return juce::Optional<GLuint>();
    }

    std::vector<GLuint> audioBufferIdentifiersBucket = foundHash->second;

    // for each hash item iterate and test full equality
    for (size_t i = 0; i < audioBufferIdentifiersBucket.size(); i++)
    {
        BufferPtr bucketAudioBuffer = getAudioBufferFromTextureId(audioBufferIdentifiersBucket[i]);
        if (bucketAudioBuffer.get() == nullptr)
        {
            std::cerr << "a TextureManager hash bucket had a GLuint texture identifier for which no audio was found"
                      << std::endl;
            continue;
        }

        if (areAudioBufferEqual(*buffer->getAudioSampleBuffer(), *bucketAudioBuffer.get()->getAudioSampleBuffer()))
        {
            return audioBufferIdentifiersBucket[i];
        }
    }

    return juce::Optional<GLuint>();
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

BufferPtr TextureManager::getAudioBufferFromTextureId(GLuint id)
{
    auto textureSearchIterator = audioBufferTextureData.find(id);

    if (textureSearchIterator == audioBufferTextureData.end())
    {
        return BufferPtr();
    }

    return textureSearchIterator->second->audioData;
}

std::shared_ptr<std::vector<float>> TextureManager::getTextureDataFromId(GLuint identifier)
{
    auto textureSearchIterator = audioBufferTextureData.find(identifier);

    if (textureSearchIterator == audioBufferTextureData.end())
    {
        return nullptr;
    }

    return textureSearchIterator->second->textureData;
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

bool TextureManager::decrementUsageCount(GLuint id)
{
    auto textureSearchIterator = audioBufferTextureData.find(id);

    if (textureSearchIterator == audioBufferTextureData.end())
    {
        std::cerr << "Warning: trying to decrement usage count of a texture id that doesn't exists" << std::endl;
        return true;
    }

    textureSearchIterator->second->useCount--;

    if (textureSearchIterator->second->useCount < 0)
    {
        std::cerr << "Warning: texture usage count went negative" << std::endl;
        return true;
    }

    if (textureSearchIterator->second->useCount == 0)
    {
        clearTextureData(id);
        return true;
    }

    return false;
}

void TextureManager::clearTextureData(GLuint textureId)
{
    auto textureSearchIterator = audioBufferTextureData.find(textureId);

    if (textureSearchIterator == audioBufferTextureData.end())
    {
        std::cerr << "Warning: trying to clear a texture id that doesn't exists" << std::endl;
        return;
    }

    // first we need to remove the texture count per audio buffer length
    auto audioBuffer = textureSearchIterator->second->audioData;
    auto sampleLength = audioBuffer->getAudioSampleBuffer()->getNumSamples();

    if (texturesLengthCount.find(sampleLength) == texturesLengthCount.end())
    {
        std::cerr << "Warning: texture to delete had no corresponding length count" << std::endl;
    }

    texturesLengthCount[sampleLength] = texturesLengthCount[sampleLength] - 1;
    if (texturesLengthCount[sampleLength] <= 0)
    {
        texturesLengthCount.erase(sampleLength);
    }

    // then we remove the glint from the bucket corresponding to the hash
    // compute hash
    int hashingLength = juce::jmin(audioBuffer->getAudioSampleBuffer()->getNumSamples(), TEXTURE_MANAGER_HASH_LENGTH);
    size_t hash = hashAudioChannel(audioBuffer->getAudioSampleBuffer()->getReadPointer(0), hashingLength);

    // if no items for this hash, this sucks really hard
    auto foundHash = texturesPerHash.find(hash);
    if (foundHash == texturesPerHash.end())
    {
        std::cerr << "A texture to delete had no corresponding hash bucket!" << std::endl;
    }
    else
    {
        bool clearedIdFromBucket = false;
        std::vector<GLuint> audioBufferIdentifiersBucket = foundHash->second;
        for (size_t i = 0; i < audioBufferIdentifiersBucket.size(); i++)
        {
            if (audioBufferIdentifiersBucket[i] == textureId)
            {
                audioBufferIdentifiersBucket.erase(audioBufferIdentifiersBucket.begin() + i);
                clearedIdFromBucket = true;
                break;
            }
        }
        if (!clearedIdFromBucket)
        {
            std::cerr << "A texture to delete was not found in its hash bucket !!" << std::endl;
        }
    }

    // finally we remove the structure with the texture data (note we already checked id exists in there)
    audioBufferTextureData.erase(textureId);
}

void TextureManager::setTexture(GLuint textureId, std::shared_ptr<SamplePlayer> sp)
{
    // increments textureLength count
    int length = sp->getBufferRef()->getAudioSampleBuffer()->getNumSamples();
    if (texturesLengthCount.find(length) == texturesLengthCount.end())
    {
        texturesLengthCount[length] = 1;
    }
    else
    {
        texturesLengthCount[length]++;
    }

    // add texture identifier to audio hash bucket
    int hashingLength =
        juce::jmin(sp->getBufferRef()->getAudioSampleBuffer()->getNumSamples(), TEXTURE_MANAGER_HASH_LENGTH);
    size_t hash = hashAudioChannel(sp->getBufferRef()->getAudioSampleBuffer()->getReadPointer(0), hashingLength);
    // if no items for this hash, create a bucket
    auto foundHash = texturesPerHash.find(hash);
    if (foundHash == texturesPerHash.end())
    {
        std::vector<GLuint> bucket;
        bucket.push_back(textureId);
        texturesPerHash[hash] = bucket;
    }
    // if a bucket exists, append to it
    else
    {
        foundHash->second.push_back(textureId);
    }

    // populate the struct with texture data and save it
    auto newTextureData = std::make_shared<AudioBufferTextureData>();
    newTextureData->audioData = sp->getBufferRef();
    newTextureData->textureData = sp->getFftData();
    newTextureData->useCount = 1;

    audioBufferTextureData.insert(
        std::pair<GLuint, std::shared_ptr<AudioBufferTextureData>>(textureId, newTextureData));
}

void TextureManager::declareTextureUsage(GLuint textureId)
{
    auto textureSearchIterator = audioBufferTextureData.find(textureId);

    if (textureSearchIterator == audioBufferTextureData.end())
    {
        std::cerr << "Warning: trying to increment count for a texture id that doesn't exists" << std::endl;
        return;
    }

    textureSearchIterator->second->useCount++;
}