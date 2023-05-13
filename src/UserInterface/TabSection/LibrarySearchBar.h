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
        setJustification(juce::Justification::left);
        setDescription("Search for files here");
        setMultiLine(false);
        setColour(juce::TextEditor::backgroundColourId, juce::Colour(COLOR_APP_BACKGROUND));
        setColour(juce::TextEditor::outlineColourId, juce::Colour::fromRGBA(255, 255, 255, 100));
        setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour::fromRGBA(255, 255, 255, 125));
        setColour(juce::TextEditor::textColourId, COLOR_TEXT);
        setMouseCursor(juce::MouseCursor::IBeamCursor);
    };
};

#endif // DEF_LIBSEARCHBAR_HPP