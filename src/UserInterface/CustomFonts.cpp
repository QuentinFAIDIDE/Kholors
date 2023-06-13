#include "CustomFonts.h"

#include "FragmentMonoFont.h"

CustomFonts::CustomFonts()
{
    monospaceFont = juce::Typeface::createSystemTypefaceFor(FragmentMonoFont::FragmentMonoRegular_ttf,
                                                            FragmentMonoFont::FragmentMonoRegular_ttfSize);
}