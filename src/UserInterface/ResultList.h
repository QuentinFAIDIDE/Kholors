#ifndef RESULT_LIST_HPP
#define RESULT_LIST_HPP

#include <juce_gui_extra/juce_gui_extra.h>

#include "../Config.h"

#define MAX_RESULT_ENTRY_NAME_SIZE 50

class ResultList : public juce::ListBoxModel {
 public:
  int getNumRows() override { return _content.size(); }
  void paintListBoxItem(int rowNumber, juce::Graphics& g, int widtht,
                        int height, bool selected) override {
    if (rowNumber % 2 == 0) {
      g.setColour(COLOR_OPAQUE_BICOLOR_LIST_1);
    } else {
      g.setColour(COLOR_OPAQUE_BICOLOR_LIST_2);
    }
    g.fillAll();
    g.setColour(COLOR_NOTIF_TEXT);
    std::string shortedName = _content[rowNumber];
    if (shortedName.size() > MAX_RESULT_ENTRY_NAME_SIZE) {
      shortedName =
          shortedName.substr(shortedName.size() - MAX_RESULT_ENTRY_NAME_SIZE,
                             MAX_RESULT_ENTRY_NAME_SIZE);
    }
    g.drawText(shortedName, g.getClipBounds(), juce::Justification::centredLeft,
               true);
  }
  void setContent(std::vector<std::string> newContent) {
    _content = newContent;
  }

 private:
  std::vector<std::string> _content;
};

#endif  // RESULT_LIST_HPP