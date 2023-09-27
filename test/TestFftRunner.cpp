#include "../src/Audio/FftRunner.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>

int main()
{
    FftRunner runner;

    //////////////////////////////////////////////////////////////////////////////////////
    //// First test, we load a sample for a 220Hz sine that is peaking at -9dB and check
    //// that the freq bins reaching max intensity are for this frequency.
    //// We do not test intensity in itself, as I'm having trouble finding resources
    //// or even a common result in other softwares as to what intensity the sine
    //// should have for the bin in the freq domain.
    //// Note that we reach as of today -14.8db, while BitWig fft for example gives another value.
    //////////////////////////////////////////////////////////////////////////////////////

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
    std::cout << "start transforming" << std::endl;
    auto result = runner.performFft(bufferPtr);
    std::cout << "stop transforming" << std::endl;

    // test result size
    if (result.get()->size() != FFT_OUTPUT_NO_FREQS *
                                    (unsigned int)runner.getNumFftFromNumSamples(reader->lengthInSamples) *
                                    reader->numChannels)
    {
        std::cout << "bad size " << result.get()->size() << std::endl;
        return 1;
    }

    float maxIntensity = -600;
    int maxIntensityFreq = 0;
    int expectedMaxIntensityFreq = float(220.0 / 22050) * float(FFT_OUTPUT_NO_FREQS);

    // test result content
    for (size_t i = 0; i < FFT_OUTPUT_NO_FREQS; i++)
    {
        float res = *(float *)(result.get()->data() + ((FFT_OUTPUT_NO_FREQS * 250) + i));

        if (res > maxIntensity)
        {
            maxIntensity = res;
            maxIntensityFreq = i;
        }

        if (std::abs(res + 64.f) > 20.f)
            std::cout << "result intensity " << i << ": " << res << std::endl;
    }

    if (std::abs(maxIntensityFreq - expectedMaxIntensityFreq) > 3)
    {
        std::cout << "Intensity peak is not corresponding to freq bin for 220Hz (test data sine freq): "
                  << maxIntensityFreq << std::endl;
        return 1;
    }

    /////////////////////////////////////////////////////////////////////////////////
    /// 2nd test, we try to check that unlike what was seen in the app at some point
    /// a kick sample doesn't have garbage data at the very end of each channel.
    /////////////////////////////////////////////////////////////////////////////////

    juce::File testTonality2("../test/TestSamples/kick.wav");
    std::unique_ptr<juce::AudioFormatReader> reader2(formatManager.createReaderFor(testTonality2));
    if (reader2.get() == nullptr)
    {
        std::cerr << "reader2 didn't like our test wav file" << std::endl;
        return 1;
    }
    // allocate a buffer
    auto bufferPtr2 = std::make_shared<juce::AudioSampleBuffer>(reader2->numChannels, reader2->lengthInSamples);
    // read file into buffer
    reader2->read(bufferPtr2.get(), 0, (int)reader2->lengthInSamples, 0, true, true);
    std::cout << "start transforming" << std::endl;
    auto result2 = runner.performFft(bufferPtr2);
    std::cout << "stop transforming" << std::endl;

    int channelFftNum = runner.getNumFftFromNumSamples(reader2->lengthInSamples);
    int lenSubdiv = (1.0 / 10.0) * channelFftNum;

    // we first reject if the size is not the right one
    if ((unsigned int)channelFftNum * FFT_OUTPUT_NO_FREQS * reader2->numChannels != result2->size())
    {
        std::cout << "Unexpected kick buffer size: " << result2->size() << std::endl;
        return 1;
    }

    std::cout << "num fft per channel in second test: " << channelFftNum << std::endl;

    // will print the intensity at 220Hz on both channel at all ffts
    for (size_t i = 0; i < reader2->numChannels; i++)
    {
        size_t channelPos = i * (size_t)channelFftNum;
        std::cout << "intensities for channel " << i << std::endl;
        for (size_t j = 0; j < (size_t)channelFftNum; j++)
        {
            size_t fft_position = (channelPos + j) * FFT_OUTPUT_NO_FREQS;
            float res = *(float *)(result2.get()->data() + (fft_position + (size_t)expectedMaxIntensityFreq));
            std::cout << res << " ";
        }
        std::cout << std::endl;
    }

    // assert that the last 10th of the sample are clear of any significant intensity on left channel
    for (int i = channelFftNum - lenSubdiv; i < channelFftNum; i++)
    {
        size_t fftPosition = (size_t)i * FFT_OUTPUT_NO_FREQS;
        // iterate over the frequencies to detect anomalies
        for (size_t j = 0; j < FFT_OUTPUT_NO_FREQS; j++)
        {
            float res = *(float *)(result2.get()->data() + (fftPosition + j));
            if (res > -20.0f)
            {
                std::cout << "got suspicious intensity at fft " << i << " with value " << res << std::endl;
                return 1;
            }
        }
    }

    return 0;
}