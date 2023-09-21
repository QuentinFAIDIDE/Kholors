#include "../src/OpenGL/TextureManager.h"
#include <memory>

std::shared_ptr<SamplePlayer> loadFile(std::string path)
{
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
        return nullptr;
    }

    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(testTonality));

    if (reader.get() == nullptr)
    {
        std::cerr << "reader didn't like our test wav file" << std::endl;
        return nullptr;
    }

    // allocate a buffer
    auto bufferPtr = std::make_shared<juce::AudioSampleBuffer>(reader->numChannels, reader->lengthInSamples);

    // read file into buffer
    reader->read(bufferPtr.get(), 0, (int)reader->lengthInSamples, 0, true, true);

    auto emptyFft = std::make_shared<std::vector<float>>();

    AudioFileBufferRef newBuffer(bufferPtr, testTonality.getFullPathName().toStdString(), emptyFft);

    auto newSample = std::make_shared<SamplePlayer>(0);
    newSample->setBuffer(newBuffer);
    newSample->setGainRamp(0.0f);
    // set initial position
    newSample->setNextReadPosition(0);
    return newSample;
}

int main()
{

    auto sp1 = loadFile("../test/TestSamples/A-sines-stereo.wav");
    auto sp2 = loadFile("../test/TestSamples/rise-up-sine.wav");
    auto sp3 = loadFile("../test/TestSamples/rise-up-sine.wav");

    sp3->getBufferRef().data->getWritePointer(0)[1024] = 0.1f;
    sp3->getBufferRef().data->getWritePointer(0)[1025] = 0.3f;

    TextureManager textureManager;
    textureManager.setOpenGlContext(nullptr); /**< set garbage just to avoid exceptions */

    // these two request should return nothing
    if (textureManager.getTextureIdentifier(sp1).hasValue())
    {
        std::cerr << "textureManager returned textureId to nonexisting texture" << std::endl;
        return 1;
    }

    if (textureManager.getTextureIdentifier(sp2).hasValue())
    {
        std::cerr << "textureManager returned textureId to nonexisting texture (2)" << std::endl;
        return 1;
    }

    textureManager.setTexture(10, sp1, sp1->getFftData());
    textureManager.setTexture(11, sp2, sp1->getFftData());

    bool foundSome = textureManager.getTextureIdentifier(sp1).hasValue();
    unsigned int foundId = *textureManager.getTextureIdentifier(sp1);
    if (!foundSome || foundId != 10)
    {
        std::cerr << "textureManager returned bad texture id 1" << std::endl;
        return 1;
    }

    if (!textureManager.getTextureIdentifier(sp2).hasValue() || *textureManager.getTextureIdentifier(sp2) != 11)
    {
        std::cerr << "textureManager returned bad texture id 2" << std::endl;
        return 1;
    }

    if (textureManager.getTextureIdentifier(sp3))
    {
        std::cerr << "Altered copy of sp1 was matched to it despise different signal values" << std::endl;
        return 1;
    }

    textureManager.declareTextureUsage(10);
    textureManager.declareTextureUsage(10);

    textureManager.decrementUsageCount(10);
    if (!textureManager.textureIdIsStored(10))
    {
        std::cerr << "Wrong freeing of sp1" << std::endl;
        return 1;
    }

    textureManager.decrementUsageCount(10);
    if (!textureManager.textureIdIsStored(10))
    {
        std::cerr << "Wrong freeing of sp1" << std::endl;
        return 1;
    }

    textureManager.decrementUsageCount(10);
    if (textureManager.textureIdIsStored(10))
    {
        std::cerr << "Missed freeing of sp1" << std::endl;
        return 1;
    }

    if (textureManager.getTextureIdentifier(sp3).hasValue())
    {
        std::cerr << "Altered texture was matched to wrong one" << std::endl;
        return 1;
    }

    if (!textureManager.getTextureIdentifier(sp2).hasValue())
    {
        std::cerr << "texture was not matched to recorded one" << std::endl;
        return 1;
    }

    return 0;
}
