#include <iostream>

#include "../src/Audio/UnitConverter.h"
#include "../src/Config.h"

int main()
{
    float diff = std::abs(UnitConverter::fftToDbInv(UnitConverter::fftToDb(200)) - 200);
    if (diff > 1)
    {
        std::cerr << "diff too big for gainToDbInv: " << diff << std::endl;
        return 1;
    }
    else
    {
        std::cerr << "fftToDb and fftToDbInv matched!" << std::endl;
    }

    diff = std::abs(UnitConverter::magnifyFftIndexInv(UnitConverter::magnifyFftIndex(3000)) - 3000);
    if (diff > 1)
    {
        std::cerr << "diff too big for magnifyFftIndex: " << diff << std::endl;
        return 1;
    }
    else
    {
        std::cerr << "magnifyFftIndex and magnifyFftIndexInv matched!" << std::endl;
    }

    diff = std::abs(UnitConverter::polylensInv(UnitConverter::polylens(0.66)) - 0.66);
    if (diff > 0.001)
    {
        std::cerr << "diff too big for polylens: " << diff << std::endl;
        return 1;
    }
    else
    {
        std::cerr << "polylens and polylensInv matched!" << std::endl;
    }

    diff = std::abs(UnitConverter::polylensInv(UnitConverter::polylens(0.995)) - 0.995);
    if (diff > 0.001)
    {
        std::cerr << "diff too big for polylens: " << diff << std::endl;
        return 1;
    }
    else
    {
        std::cerr << "polylens and polylensInv matched!" << std::endl;
    }

    diff = std::abs(UnitConverter::polylensInv(UnitConverter::polylens(0.9765625)) - 0.9765625);
    if (diff > 0.001)
    {
        std::cerr << "diff too big for polylens: " << diff << std::endl;
        return 1;
    }
    else
    {
        std::cerr << "polylens and polylensInv matched!" << std::endl;
    }

    diff = std::abs(UnitConverter::polylensInv(UnitConverter::polylens(0.1221001221001221)) - 0.1221001221001221);
    if (diff > 0.001)
    {
        std::cerr << "diff too big for polylens: " << diff << std::endl;
        return 1;
    }
    else
    {
        std::cerr << "polylens and polylensInv matched!" << std::endl;
    }

    diff = std::abs(UnitConverter::sigmoidInv(UnitConverter::sigmoid(0.66)) - 0.66);
    if (diff > 0.001)
    {
        std::cerr << "diff too big for sigmoid: " << diff << std::endl;
        return 1;
    }
    else
    {
        std::cerr << "sigmoid and sigmoidInv matched!" << std::endl;
    }

    int magnifiedIndex = UnitConverter::magnifyTextureFrequencyIndex(500);
    diff = std::abs(UnitConverter::magnifyTextureFrequencyIndexInv(magnifiedIndex) - 500);
    if (diff > 1)
    {
        std::cerr << "diff too big for magnifyTextureFrequencyIndex 500: " << diff << std::endl;
        return 1;
    }
    else
    {
        std::cerr << "magnifyTextureFrequencyIndex and magnifyTextureFrequencyIndexInv matched!" << std::endl;
    }

    magnifiedIndex = UnitConverter::magnifyTextureFrequencyIndex(4000);
    diff = std::abs(UnitConverter::magnifyTextureFrequencyIndexInv(magnifiedIndex) - 4000);
    if (diff > 1)
    {
        std::cerr << "diff too big for magnifyTextureFrequencyIndex 4000: " << diff << std::endl;
        return 1;
    }
    else
    {
        std::cerr << "magnifyTextureFrequencyIndex and magnifyTextureFrequencyIndexInv matched!" << std::endl;
    }

    // testing the whole pipeline to go back and forth at 4.6kHz
    int index4600 = (4600.0f / float(AUDIO_FRAMERATE >> 1)) * float(FREQVIEW_SAMPLE_FFT_SIZE);
    int indexStored = UnitConverter::magnifyFftIndexInv(index4600);
    int indexTexture = UnitConverter::magnifyTextureFrequencyIndex(indexStored);
    indexStored = UnitConverter::magnifyTextureFrequencyIndexInv(indexTexture);
    int retrievedIndex4600 = UnitConverter::magnifyFftIndex(indexStored);
    diff = index4600 - retrievedIndex4600;
    if (diff >= 2)
    {
        std::cerr << "fft to texture pipeline has too much diff on freq 4600: " << diff << std::endl;
        return 1;
    }
}