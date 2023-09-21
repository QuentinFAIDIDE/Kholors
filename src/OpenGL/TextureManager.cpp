#include "TextureManager.h"
#include <limits>
#include <memory>
#include <stdexcept>

juce::Optional<GLuint> TextureManager::getTextureIdentifier(std::shared_ptr<SamplePlayer> sp)
{
    AudioFileBufferRef buffer = sp->getBufferRef();

    // if opengl context to free textures was not set, throw an error
    if (!glContext.has_value())
    {
        throw std::runtime_error("Called a texture manager function before setting opengl context");
    }

    // if length is unique, directly return nothing
    if (texturesLengthCount.find(buffer.data->getNumSamples()) == texturesLengthCount.end())
    {
        return juce::Optional<GLuint>();
    }

    // compute hash
    int hashingLength = juce::jmin(buffer.data->getNumSamples(), TEXTURE_MANAGER_HASH_LENGTH);
    size_t hash = hashAudioChannel(buffer.data->getReadPointer(0), hashingLength);

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
        AudioFileBufferRef bucketAudioBuffer = getAudioBufferFromTextureId(audioBufferIdentifiersBucket[i]);
        if (bucketAudioBuffer.data == nullptr)
        {
            throw std::runtime_error(
                "a TextureManager hash bucket had a GLuint texture identifier for which no audio was found");
        }

        if (areAudioBufferEqual(*buffer.data, *bucketAudioBuffer.data))
        {
            return audioBufferIdentifiersBucket[i];
        }
    }

    return juce::Optional<GLuint>();
}

void TextureManager::setOpenGlContext(juce::OpenGLContext *gl)
{
    glContext = gl;
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

    for (size_t chan = 0; chan < (size_t)a.getNumChannels(); chan++)
    {
        auto aData = a.getReadPointer(chan);
        auto bData = b.getReadPointer(chan);

        for (size_t sample = 0; sample < (size_t)a.getNumSamples(); sample++)
        {
            if (std::abs(aData[sample] - bData[sample]) > std::numeric_limits<float>::epsilon())
            {
                return false;
            }
        }
    }

    return true;
}

AudioFileBufferRef TextureManager::getAudioBufferFromTextureId(GLuint id)
{
    auto textureSearchIterator = audioBufferTextureData.find(id);

    // NOTE: it would be better to use a std::optional here rather than making a fake buffer
    if (textureSearchIterator == audioBufferTextureData.end())
    {
        return AudioFileBufferRef();
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
    for (size_t i = 0; i < (size_t)length; i++)
    {
        seed ^= hasher(data[i]) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    return seed;
}

void TextureManager::decrementUsageCount(GLuint id)
{
    // if opengl context to free textures was not set, throw an error
    if (!glContext.has_value())
    {
        throw std::runtime_error("Called a texture manager function before setting opengl context");
    }

    auto textureSearchIterator = audioBufferTextureData.find(id);

    if (textureSearchIterator == audioBufferTextureData.end())
    {
        throw std::runtime_error("trying to decrement usage count of a texture id that doesn't exists");
    }

    textureSearchIterator->second->useCount--;

    if (textureSearchIterator->second->useCount < 0)
    {
        throw std::runtime_error("texture usage count went negative");
    }

    if (textureSearchIterator->second->useCount == 0)
    {
        clearTextureData(id);
    }
}

bool TextureManager::textureIdIsStored(GLuint textureId)
{
    return audioBufferTextureData.find(textureId) != audioBufferTextureData.end();
}

void TextureManager::clearTextureData(GLuint textureId)
{
    std::cout << "Deleting texture for GLuint " << textureId << std::endl;

    // if opengl context to free textures was not set, throw an error
    if (!glContext.has_value())
    {
        throw std::runtime_error("Called a texture manager function before setting opengl context");
    }

    // ensure the texture exists
    auto textureSearchIterator = audioBufferTextureData.find(textureId);
    if (textureSearchIterator == audioBufferTextureData.end())
    {
        throw std::runtime_error("trying to clear a texture id that doesn't exists");
    }

    // first we need to remove the texture count per audio buffer length
    auto audioBuffer = textureSearchIterator->second->audioData;
    auto sampleLength = audioBuffer.data->getNumSamples();
    if (texturesLengthCount.find(sampleLength) == texturesLengthCount.end())
    {
        throw std::runtime_error("texture to delete had no corresponding length count");
    }
    texturesLengthCount[sampleLength] = texturesLengthCount[sampleLength] - 1;
    if (texturesLengthCount[sampleLength] <= 0)
    {
        texturesLengthCount.erase(sampleLength);
    }

    // then we remove the glint from the bucket corresponding to the hash
    int hashingLength = juce::jmin(audioBuffer.data->getNumSamples(), TEXTURE_MANAGER_HASH_LENGTH);
    size_t hash = hashAudioChannel(audioBuffer.data->getReadPointer(0), hashingLength);
    // if no items for this hash, this sucks really hard
    auto foundHash = texturesPerHash.find(hash);
    if (foundHash == texturesPerHash.end())
    {
        throw std::runtime_error("A texture to delete had no corresponding hash bucket!");
    }
    else
    {
        bool clearedIdFromBucket = false;
        std::vector<GLuint> audioBufferIdentifiersBucket = foundHash->second;
        for (size_t i = 0; i < audioBufferIdentifiersBucket.size(); i++)
        {
            if (audioBufferIdentifiersBucket[i] == textureId)
            {
                audioBufferIdentifiersBucket.erase(audioBufferIdentifiersBucket.begin() + (long)i);
                clearedIdFromBucket = true;
                // as we modify the copy, we need to copy it back!
                texturesPerHash[hash] = audioBufferIdentifiersBucket;
                break;
            }
        }
        if (!clearedIdFromBucket)
        {
            throw std::runtime_error("A texture to delete was not found in its hash bucket !!");
        }
    }

    // we remove the structure with the texture data (note we already checked id exists in there)
    audioBufferTextureData.erase(textureId);

// we only proceed to call opengl if we are not in testing mode
#ifndef WITH_TESTING
    // and we execute a call on the openGlThread to free the texture
    glContext.value()->executeOnGLThread(
        [textureId](juce::OpenGLContext &) { juce::gl::glDeleteTextures(1, &textureId); }, true);
#endif
}

void TextureManager::setTexture(GLuint textureId, std::shared_ptr<SamplePlayer> sp,
                                std::shared_ptr<std::vector<float>> textureData)
{
    std::cout << "Storing texture for GLuint " << textureId << std::endl;

    // if opengl context to free textures was not set, throw an error
    if (!glContext.has_value())
    {
        throw std::runtime_error("Called a texture manager function before setting opengl context");
    }

    // increments textureLength count
    int length = sp->getBufferRef().data->getNumSamples();
    if (texturesLengthCount.find(length) == texturesLengthCount.end())
    {
        texturesLengthCount[length] = 1;
    }
    else
    {
        texturesLengthCount[length]++;
    }

    // add texture identifier to audio hash bucket
    int hashingLength = juce::jmin(sp->getBufferRef().data->getNumSamples(), TEXTURE_MANAGER_HASH_LENGTH);
    size_t hash = hashAudioChannel(sp->getBufferRef().data->getReadPointer(0), hashingLength);
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
    newTextureData->textureData = textureData;
    newTextureData->useCount = 1;

    audioBufferTextureData.insert(
        std::pair<GLuint, std::shared_ptr<AudioBufferTextureData>>(textureId, newTextureData));
}

void TextureManager::declareTextureUsage(GLuint textureId)
{
    // if opengl context to free textures was not set, throw an error
    if (!glContext.has_value())
    {
        throw std::runtime_error("Called a texture manager function before setting opengl context");
    }

    auto textureSearchIterator = audioBufferTextureData.find(textureId);

    if (textureSearchIterator == audioBufferTextureData.end())
    {
        throw std::runtime_error("Warning: trying to increment count for a texture id that doesn't exists");
    }

    textureSearchIterator->second->useCount++;
}

void TextureManager::lockCurrentTextures()
{
    if (lockedTextures.size() > 0)
    {
        throw std::runtime_error("tried to lock textures multiple times");
    }
}

void TextureManager::releaseLockedTextures()
{
    std::set<GLuint> texturesToFree;
    texturesToFree.swap(lockedTextures);
    for (auto it = texturesToFree.begin(); it != texturesToFree.end(); it++)
    {
        decrementUsageCount(*it);
    }
}
