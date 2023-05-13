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
        g.setColour(COLOR_APP_BACKGROUND);
        g.fillAll();

        g.setColour(COLOR_TEXT);

        if (selected)
        {
            g.drawRect(g.getClipBounds(), 1);
        }

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