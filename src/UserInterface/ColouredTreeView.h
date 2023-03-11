#ifndef COLOURED_TREE_VIEW_HPP
#define COLOURED_TREE_VIEW_HPP

#include <juce_gui_extra/juce_gui_extra.h>

class ColouredTreeView : public juce::TreeView {
  void paint(juce::Graphics& g) {
    juce::Rectangle<int> bounds = g.getClipBounds();
    bounds.setHeight(getIndentSize());

    int i = 0;

    int shift = bounds.getHeight() -
                (getViewport()->getViewPositionY() % bounds.getHeight());
    bounds.setY(bounds.getY() + shift);

    while (bounds.getY() <
           g.getClipBounds().getY() + g.getClipBounds().getHeight()) {
      if (i % 2 == 0) {
        g.setColour(juce::Colour::fromFloatRGBA(0.2, 0.2, 0.2, 0.1));
        g.fillRect(bounds);
      }

      g.setColour(juce::Colour::fromFloatRGBA(1, 1, 1, 0.4));
      g.drawLine(bounds.getX(), bounds.getY(),
                 bounds.getX() + bounds.getWidth(), bounds.getY(), 0.2);

      bounds.setY(bounds.getY() + bounds.getHeight());
      i++;
    }
  }
};

#endif  // COLOURED_TREE_VIEW_HPP