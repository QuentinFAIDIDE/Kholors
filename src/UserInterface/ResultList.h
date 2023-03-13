#ifndef RESULT_LIST_HPP
#define RESULT_LIST_HPP

#include <juce_gui_extra/juce_gui_extra.h>

#include "../Config.h"

#define MAX_RESULT_ENTRY_NAME_SIZE 50

class ResultList : public juce::ListBoxModel {
 public:
  ResultList() : _currentSelection(-1) {}

  int getNumRows() override { return _content.size(); }

  void resetSelect() { _currentSelection = -1; }

  void paintListBoxItem(int rowNumber, juce::Graphics& g, int widtht,
                        int height, bool selected) override {
    if (rowNumber % 2 == 0) {
      g.setColour(COLOR_OPAQUE_BICOLOR_LIST_1);
    } else {
      g.setColour(COLOR_OPAQUE_BICOLOR_LIST_2);
    }
    g.fillAll();

    g.setColour(COLOR_NOTIF_TEXT);

    if (selected) {
      g.drawRect(g.getClipBounds(), 1);
    }

    std::string shortedName = _content[rowNumber];

    bool nameShortened = false;
    while (_defaultFont.getStringWidth("..." + shortedName) >= widtht - 10) {
      shortedName = shortedName.substr(4, shortedName.size() - 4);
      nameShortened = true;
    }

    if (nameShortened) {
      shortedName = "..." + shortedName;
    }

    g.drawText(shortedName, g.getClipBounds().reduced(5, 0),
               juce::Justification::centredLeft, false);
  }

  void setContent(std::vector<std::string> newContent) {
    _content = newContent;
  }

  void listBoxItemClicked(int row, const juce::MouseEvent& me) override {
    _currentSelection = row;
  }

  void listBoxItemDoubleClicked(int row, const juce::MouseEvent& me) override {
    focusElementCallback(_content[row]);
  }

  void backgroundClicked(const juce::MouseEvent&) override {
    _currentSelection = -1;
  }

  std::function<void(std::string)> focusElementCallback;

 private:
  std::vector<std::string> _content;
  int _currentSelection;
  juce::Font _defaultFont;
};

#endif  // RESULT_LIST_HPP