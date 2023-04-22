#ifndef COLOURED_TREE_VIEW_HPP
#define COLOURED_TREE_VIEW_HPP

#include <juce_gui_extra/juce_gui_extra.h>

#include "../Config.h"

class ColouredTreeView : public juce::TreeView
{
    void paint(juce::Graphics &g)
    {
        juce::Rectangle<int> bounds = g.getClipBounds();
        bounds.setHeight(getIndentSize());

        int i = 0;

        int shift = bounds.getHeight() - (getViewport()->getViewPositionY() % bounds.getHeight());
        bounds.setY(bounds.getY() + shift);

        while (bounds.getY() < g.getClipBounds().getY() + g.getClipBounds().getHeight())
        {
            if (i % 2 == 0)
            {
                g.setColour(COLOR_OPAQUE_BICOLOR_LIST_1);
                g.fillRect(bounds);
            }

            g.setColour(COLOR_OPAQUE_BICOLOR_LIST_2);
            g.drawLine(bounds.getX(), bounds.getY(), bounds.getX() + bounds.getWidth(), bounds.getY(), 0.2);

            bounds.setY(bounds.getY() + bounds.getHeight());
            i++;
        }
    }
};

#endif // COLOURED_TREE_VIEW_HPP