#include "../src/Audio/FftRunner.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>

int main()
{
    FftRunner runner;

    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    juce::File testTonality("TestSamples/A_220Hz_9dB.wav");
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
    auto result = runner.performFft(bufferPtr);

    // TODO: test result size
    // TODO: test result content

    return 0;
}