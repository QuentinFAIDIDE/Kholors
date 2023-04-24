#include <iostream>

#include "../src/Audio/UnitConverter.h"

int main()
{
    float diff = std::abs(UnitConverter::fftToDbInv(UnitConverter::fftToDb(200)) - 200);
    if (diff > 1)
    {
        std::cerr << "diff too big for gainToDbInv: " << diff << std::endl;
    }
    else
    {
        std::cerr << "fftToDb and fftToDbInv matched!" << std::endl;
    }

    return 0;
}