#include "ColorPicker.h"
#include "../Section.h"

#include "../../Arrangement/ColorPalette.h"

// full color square size including margins
#define COLORSQUARE_SIZE 25
#define COLORSQUARE_INNER_MARGINS 3

ColorPicker::ColorPicker(ActivityManager &am) : activityManager(am)
{
}

void ColorPicker::paint(juce::Graphics &g)
{
    auto bounds = g.getClipBounds();

    juce::Colour bg = juce::Colours::transparentBlack;
    drawSection(g, bounds, "Group Color", bg);

    // get the bounds below the section title
    auto boxesArea = bounds;
    boxesArea.removeFromTop(SECTION_TITLE_HEIGHT);
    boxesArea.reduce(4, 4);

    // compute how many squares will fit
    int noHorizontalSqares = (boxesArea.getWidth() / COLORSQUARE_SIZE);
    int noVerticalSquares = (boxesArea.getHeight() / COLORSQUARE_SIZE);

    // reduce the box depending on the leftover pixels (so that we have a perfect fitting grid)
    int xLeftoverPixels = boxesArea.getWidth() - (COLORSQUARE_SIZE * noHorizontalSqares);
    boxesArea.reduce(xLeftoverPixels / 2, 0);

    // limit the number of colors display to those we have squares for
    int noColors = colourPalette.size();
    int noPossibleColors = noHorizontalSqares * noVerticalSquares;
    int noDisplayedColors = juce::jmin(noPossibleColors, noColors);

    // iterate over squares and dram em
    for (int i = 0; i < noDisplayedColors; i++)
    {
        int horizontalIndex = i % noHorizontalSqares;
        int verticalIndex = i / noHorizontalSqares;
        auto square = boxesArea;
        square.setWidth(COLORSQUARE_SIZE);
        square.setHeight(COLORSQUARE_SIZE);
        square = square.withX(square.getX() + horizontalIndex * COLORSQUARE_SIZE);
        square = square.withY(square.getY() + verticalIndex * COLORSQUARE_SIZE);

        square.reduce(COLORSQUARE_INNER_MARGINS, COLORSQUARE_INNER_MARGINS);
        g.setColour(colourPalette[i]);
        g.fillRoundedRectangle(square.toFloat(), 2);
        g.setColour(COLOR_TEXT_DARKER);
        g.drawRoundedRectangle(square.toFloat(), 2, 1);

        colorSquares[i] = square;
    }
}

void ColorPicker::mouseUp(const juce::MouseEvent &event)
{
    std::map<int, juce::Rectangle<int>>::iterator it;
    for (it = colorSquares.begin(); it != colorSquares.end(); it++)
    {
        if (it->second.contains(event.getMouseDownPosition()))
        {
            activityManager.broadcastTask(std::make_shared<SampleGroupRecolor>(it->first));
            return;
        }
    }
}