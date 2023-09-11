#ifndef RESULT_LIST_HPP
#define RESULT_LIST_HPP

#include <juce_gui_extra/juce_gui_extra.h>

#include "../../Config.h"

#define MAX_RESULT_ENTRY_NAME_SIZE 50

class ResultList : public juce::ListBoxModel
{
  public:
    ResultList() : currentSelection(-1)
    {
    }

    int getNumRows() override
    {
        return content.size();
    }

    void resetSelect()
    {
        currentSelection = -1;
    }

    void paintListBoxItem(int rowNumber, juce::Graphics &g, int widtht, int height, bool selected) override
    {

        if (selected)
        {
            g.setColour(COLOR_SELECTED_BACKGROUND);
            g.fillRect(g.getClipBounds());
        }

        g.setColour(COLOR_TEXT_DARKER);

        std::string shortedName = content[rowNumber];

        bool nameShortened = false;
        while (defaultFont.getStringWidth("..." + shortedName) >= widtht - 10)
        {
            shortedName = shortedName.substr(4, shortedName.size() - 4);
            nameShortened = true;
        }

        if (nameShortened)
        {
            shortedName = "..." + shortedName;
        }

        g.setFont(DEFAULT_FONT_SIZE);
        g.drawText(shortedName, g.getClipBounds().reduced(5, 0), juce::Justification::centredLeft, false);
    }

    void setContent(std::vector<std::string> newContent)
    {
        content = newContent;
    }

    void emptyContent()
    {
        content.clear();
    }

    void listBoxItemClicked(int row, const juce::MouseEvent &me) override
    {
        currentSelection = row;
    }

    void listBoxItemDoubleClicked(int row, const juce::MouseEvent &me) override
    {
        focusElementCallback(content[row]);
    }

    void backgroundClicked(const juce::MouseEvent &) override
    {
        currentSelection = -1;
    }

    std::function<void(std::string)> focusElementCallback;

  private:
    std::vector<std::string> content;
    int currentSelection;
    juce::Font defaultFont;
};

#endif // RESULT_LIST_HPP