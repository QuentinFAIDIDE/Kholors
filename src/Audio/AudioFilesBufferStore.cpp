#include "AudioFilesBufferStore.h"

#include "../Config.h"
#include "FftRunner.h"
#include "UnitConverter.h"
#include <regex>
#include <stdexcept>

AudioFileBufferRef::AudioFileBufferRef() : data(nullptr), fileFullPath("")
{
}

AudioFileBufferRef::AudioFileBufferRef(std::shared_ptr<juce::AudioSampleBuffer> ptr, std::string path,
                                       std::shared_ptr<std::vector<float>> shortTimeDFTs)
    : data(ptr), fileFullPath(path)
{
    // we now allow nullptr AudioFileBufferRef
    if (ptr == nullptr)
    {
        return;
    }

    // reserve an array of data to concatenate all the channel hashes
    std::vector<unsigned char> concatenatedChanHashes;
    concatenatedChanHashes.resize(SHA_DIGEST_LENGTH * (size_t)data->getNumChannels());

    // we will first build a hash for each channel
    for (int i = 0; i < data->getNumChannels(); i++)
    {
        size_t mdlen;
        EVP_Q_digest(nullptr, "SHA1", nullptr, data->getReadPointer(i), sizeof(float) * (size_t)data->getNumSamples(),
                     concatenatedChanHashes.data() + (SHA_DIGEST_LENGTH * i), &mdlen);

        if (mdlen != SHA_DIGEST_LENGTH)
        {
            throw std::runtime_error("Unexpected size of sha digest received !");
        }
    }

    // we then hash the hashes so they are combined in a lazy manner
    // NOTE: at the scale of the huge audio files, one or two more iterations
    // are basically free.

    size_t hashlen;
    // compute digest of the file
    EVP_Q_digest(nullptr, "SHA1", nullptr, concatenatedChanHashes.data(), concatenatedChanHashes.size(), hash,
                 &hashlen);

    // we once again assert that the hash has expected size
    if (hashlen != SHA_DIGEST_LENGTH)
    {
        throw std::runtime_error("Unexpected size of sha digest received !");
    }

    // save the FFT data
    storedFftData = shortTimeDFTs;
}

std::string AudioFileBufferRef::hashDigest()
{
    std::stringstream ss;
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++)
    {
        ss << std::hex << hash[i];
    }
    std::string fullDigest;
    ss >> fullDigest;
    return fullDigest;
}

//////////////////////////////////////////////////

AudioFilesBufferStore::AudioFilesBufferStore() : allowUnusedBufferRelease(true)
{
    formatManager.registerBasicFormats();
}

AudioFileBufferRef AudioFilesBufferStore::loadSample(std::string filePath)
{
    // get the full file path on disk
    juce::File file(filePath);
    juce::String fullPath = file.getFullPathName();

    // safety measure for double slashes (windows and linux style)
    fullPath = fullPath.replace("//", "/");
    fullPath = fullPath.replace("\\\\", "\\");

    // check cache and return cached item if present
    {
        juce::ScopedLock l(lock);

        auto foundItem = audioBuffersCache.find(fullPath.toStdString());
        if (foundItem != audioBuffersCache.end())
        {
            return foundItem->second;
        }
    }

    // get a reader to have its size
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));

    // abort if a failure happened
    if (reader.get() == nullptr)
    {
        throw std::runtime_error(std::string() + "Unable to open reader for file: " + fullPath.toStdString());
    }

    // abort if size is not in allowed bounds
    auto duration = (float)reader->lengthInSamples / reader->sampleRate;
    if (duration >= SAMPLE_MAX_DURATION_SEC || reader->lengthInSamples <= SAMPLE_MIN_DURATION_FRAMES)
    {
        throw std::runtime_error(std::string() + "File had unsupported length: " + fullPath.toStdString());
    }

    // load the audio data
    auto bufferPtr = std::make_shared<juce::AudioSampleBuffer>(reader->numChannels, reader->lengthInSamples);
    reader->read(bufferPtr.get(), 0, reader->lengthInSamples, 0, true, true);

    // compute the short time FFTs
    auto rawShortTimeDfts = fftProcessing->performFft(bufferPtr);

    // apply the transformation to store FFTs
    std::shared_ptr<std::vector<float>> storedFfts = copyRawFFTsToStorageFormat(rawShortTimeDfts);

    // finally, create the buffer object
    AudioFileBufferRef bufferBox(bufferPtr, fullPath.toStdString(), storedFfts);

    // register in cache
    {
        juce::ScopedLock l(lock);

        audioBuffersCache.insert(std::pair<std::string, AudioFileBufferRef>(fullPath.toStdString(), bufferBox));
    }

    // return buffer
    return bufferBox;
}

std::shared_ptr<std::vector<float>> AudioFilesBufferStore::copyRawFFTsToStorageFormat(
    std::shared_ptr<std::vector<float>> rawFFTs)
{
    auto storedFFTs = std::make_shared<std::vector<float>>();

    // throw if the input data stinks
    if (rawFFTs->size() % FFT_OUTPUT_NO_FREQS != 0)
    {
        throw std::runtime_error("FFTs received in copyRawFFTsToStorageFormat have an unexpected size (non dividable "
                                 "by FFT_OUTPUT_NO_FREQS)");
    }

    // how many FFT we will transform ?
    int numFFT = rawFFTs->size() / FFT_OUTPUT_NO_FREQS;

    // allocate appropriate size
    storedFFTs->resize((size_t)numFFT * FFT_STORAGE_SCOPE_SIZE);

    // max fftIndex in the FFTW output
    int maxFftIndex = FFT_OUTPUT_NO_FREQS - 1;

    // proceed to iterate over transforming FFTs
    for (size_t i = 0; i < (size_t)numFFT; i++)
    {
        size_t srcFftPosition = i * FFT_OUTPUT_NO_FREQS;
        size_t dstFftPosition = i * FFT_STORAGE_SCOPE_SIZE;

        for (size_t j = 0; j < FFT_STORAGE_SCOPE_SIZE; j++)
        {

            // map the index to magnify important frequencies
            float logIndexFft = UnitConverter::magnifyFftIndex(j);

            // try to do a linear interpolation between the two indexes
            size_t belowIndex = (size_t)std::floor(logIndexFft);
            size_t aboveIndex = (size_t)std::ceil(logIndexFft);
            float interpolationPosition = logIndexFft - std::floor(logIndexFft);

            // tried to prevent reading irrelevant data due to ceiling
            if (aboveIndex > (size_t)maxFftIndex)
            {
                aboveIndex = (size_t)maxFftIndex;
            }

            if (belowIndex > (size_t)maxFftIndex)
            {
                belowIndex = (size_t)maxFftIndex;
            }

            (*storedFFTs)[dstFftPosition + j] =
                ((*rawFFTs)[srcFftPosition + belowIndex] * (1.0f - interpolationPosition)) +
                ((*rawFFTs)[srcFftPosition + aboveIndex] * (interpolationPosition));
        }
    }

    return storedFFTs;
}

void AudioFilesBufferStore::releaseUnusedBuffers()
{
    {
        juce::ScopedLock l(lock);

        if (allowUnusedBufferRelease)
        {
            // to avoid deleting a container we're iterating
            std::vector<std::string> filesToDelete;

            // iterate over cached items, and remove all those who have
            // less than two copies around
            for (auto it = audioBuffersCache.begin(); it != audioBuffersCache.end(); it++)
            {
                if (it->second.data.use_count() == 1)
                {
                    std::cout << "Unused buffer to be cleared: " << it->first << std::endl;
                    filesToDelete.push_back(it->first);
                }
            }

            for (size_t i = 0; i < filesToDelete.size(); i++)
            {
                auto fileToDelete = audioBuffersCache.find(filesToDelete[i]);
                if (fileToDelete != audioBuffersCache.end())
                {
                    audioBuffersCache.erase(fileToDelete);
                }
            }
        }
    }
}

void AudioFilesBufferStore::disableUnusedBuffersRelease()
{
    {
        juce::ScopedLock l(lock);
        allowUnusedBufferRelease = false;
    }
}

void AudioFilesBufferStore::enableUnusedBuffersRelease()
{
    {
        juce::ScopedLock l(lock);
        allowUnusedBufferRelease = true;
    }
}