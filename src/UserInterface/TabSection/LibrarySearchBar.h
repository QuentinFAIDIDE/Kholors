#ifndef DEF_LIBSEARCHBAR_HPP
#define DEF_LIBSEARCHBAR_HPP

#include "../../Config.h"
#include "../../Library/AudioLibraryManager.h"
#include "ResultList.h"
#include <juce_gui_extra/juce_gui_extra.h>

/**
 * A searchbar for the audio library tab where users can search for samples.
 */
class LibrarySearchBar : public juce::TextEditor
{
  public:
    LibrarySearchBar()
    {
        setCaretVisible(true);
        setScrollbarsShown(false);
        setJustification(juce::Justification::centredLeft);
        setDescription("Search for files here");
        setMultiLine(false);
        setMouseCursor(juce::MouseCursor::IBeamCursor);
    };
};

#endif // DEF_LIBSEARCHBAR_HPP