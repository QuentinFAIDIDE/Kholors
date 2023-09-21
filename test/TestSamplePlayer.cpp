#include "../src/Audio/SamplePlayer.h"

int testSamplePlayerWithSample(std::string path, int blockSize, int offset, int startShift)
{
    std::cerr << "testing file " << path << " with block size " << blockSize << " and offset " << offset << std::endl;

    juce::SharedResourcePointer<FftRunner> fftProcessing;

    // we need an FFT object for the samplePlayer.
    // I'm a bad man and I decided to hardcode that one here.
    // This is because importing config will add this package
    // to the numerous one that are rebuilt for importing
    // config.
    juce::dsp::FFT fft(9);

    // load wav file
    juce::File testTonality(path);

    if (!testTonality.existsAsFile())
    {
        std::cerr << "unable to find test file" << std::endl;
        return 1;
    }

    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(testTonality));

    if (reader.get() == nullptr)
    {
        std::cerr << "reader didn't like our test wav file" << std::endl;
        return 1;
    }

    // allocate a buffer
    auto bufferPtr = std::make_shared<juce::AudioSampleBuffer>(reader->numChannels, reader->lengthInSamples);

    // read file into buffer
    reader->read(bufferPtr.get(), 0, (int)reader->lengthInSamples, 0, true, true);

    // compute the short time FFTs
    auto rawShortTimeDfts = fftProcessing->performFft(bufferPtr);

    // note: we are sending the wrong FFT format, we send FFTW raw output, but in theory this buffer takes the
    // transformed/normalized version that has a different size and scale.

    AudioFileBufferRef newBuffer(bufferPtr, testTonality.getFullPathName().toStdString(), rawShortTimeDfts);

    SamplePlayer *newSample = new SamplePlayer(offset);
    newSample->setBuffer(newBuffer);
    newSample->setBufferShift(startShift);
    newSample->setGainRamp(0.0f);
    // set initial position
    newSample->setNextReadPosition(0);

    if (newBuffer.data->getNumSamples() != newSample->getTotalLength())
    {
        std::cerr << "getTotalLength differ from buffer num samples" << std::endl;
        return 1;
    }

    if (newBuffer.data->getNumSamples() - startShift != newSample->getLength())
    {
        std::cerr << "getLength differ from buffer num samples: expected " << newBuffer.data->getNumSamples()
                  << " but got " << newSample->getLength() << std::endl;
        return 1;
    }

    // now we iteratively get next audio block just like the sound card does

    // initialize size of blocks to read and position
    int testReadPosition = 0;
    int bufferSize = newBuffer.data->getNumSamples();
    // allocate a buffer that go up to next 1024 sample block above buffer size
    int testBufferSize = bufferSize + (blockSize - (bufferSize % blockSize)) + offset;

    // audio buffer to read data into
    juce::AudioBuffer<float> audioBuffer(2, testBufferSize);
    audioBuffer.clear();

    while (testReadPosition < bufferSize + offset)
    {
        const juce::AudioSourceChannelInfo audioSourceInfo(&audioBuffer, testReadPosition, blockSize);
        newSample->getNextAudioBlock(audioSourceInfo);
        testReadPosition += blockSize;
    }

    // test that the audio buffer has the same data as the original sample
    for (int i = 0; i < bufferSize - startShift; i++)
    {
        for (int chan = 0; chan < 2; chan++)
        {
            int originalAudio = newBuffer.data->getReadPointer(chan)[startShift + i];
            int retrievedAudio = audioBuffer.getReadPointer(chan)[offset + i];
            if (originalAudio != retrievedAudio)
            {
                std::cerr << "Audio data differ in channel " << chan << ". Had a sample difference at sample number "
                          << i << std::endl;
                return 1;
            }
        }
    }

    delete newSample;

    return 0;
}

int main()
{
    int retcode = 0;

    juce::SharedResourcePointer<FftRunner> fftProcessing;

    retcode = testSamplePlayerWithSample("../test/TestSamples/rise-up-sine.wav", 1024, 0, 0);
    if (retcode != 0)
    {
        return 1;
    }

    retcode = testSamplePlayerWithSample("../test/TestSamples/A-sines-stereo.wav", 1024, 0, 0);
    if (retcode != 0)
    {
        return 1;
    }

    retcode = testSamplePlayerWithSample("../test/TestSamples/rise-up-sine.wav", 1024, 100, 0);
    if (retcode != 0)
    {
        return 1;
    }

    retcode = testSamplePlayerWithSample("../test/TestSamples/A-sines-stereo.wav", 1024, 20, 0);
    if (retcode != 0)
    {
        return 1;
    }

    retcode = testSamplePlayerWithSample("../test/TestSamples/rise-up-sine.wav", 1024, 0, 354);
    if (retcode != 0)
    {
        return 1;
    }

    retcode = testSamplePlayerWithSample("../test/TestSamples/A-sines-stereo.wav", 1024, 0, 3096);
    if (retcode != 0)
    {
        return 1;
    }

    retcode = testSamplePlayerWithSample("../test/TestSamples/rise-up-sine.wav", 1024, 12, 354);
    if (retcode != 0)
    {
        return 1;
    }

    retcode = testSamplePlayerWithSample("../test/TestSamples/A-sines-stereo.wav", 1024, 540, 3096);
    if (retcode != 0)
    {
        return 1;
    }

    return 0;
}