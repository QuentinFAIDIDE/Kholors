#include "FontsLoader.h"

#include "FragmentMonoFont.h"

FontsLoader::FontsLoader()
{
    monospaceFont = juce::Typeface::createSystemTypefaceFor(FragmentMonoFont::FragmentMonoRegular_ttf,
                                                            FragmentMonoFont::FragmentMonoRegular_ttfSize);
}