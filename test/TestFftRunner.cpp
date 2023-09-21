#include "../src/Audio/FftRunner.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>

int main()
{
    FftRunner runner;

    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    juce::File testTonality("../test/TestSamples/A_220Hz_9dB.wav");
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

    // test result size
    std::cout << "result size: " << result->size() << std::endl;
    // test result content
    for (size_t i = 0; i < FFT_OUTPUT_NO_FREQS; i++)
    {
        float res = *(float *)(result.get()->data() + ((FFT_OUTPUT_NO_FREQS * 100) + i));
        if (std::abs(res + 64.f) > 20.f)
            std::cout << "result intensity " << i << ": " << res << std::endl;
    }
    return 0;
}