#ifndef DEF_UNIT_CONVERTER_HPP
#define DEF_UNIT_CONVERTER_HPP

#include "../Config.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>

class UnitConverter
{
  public:
    static float gainToDb(float val);
    static float gainToDbInv(float val);

    static int magnifyFftIndex(int k);
    static int magnifyFftIndexInv(int k);

  private:
    static float magnifyFftPrecomputedFactor1;
    static float magnifyFftPrecomputedFactor2;

    static float magnifyFftInvPrecomputedFator1;
    static float magnifyFftInvPrecomputedFator2;
    static float magnifyFftInvPrecomputedFator3;
};

#endif // DEF_UNIT_CONVERTER_HPP