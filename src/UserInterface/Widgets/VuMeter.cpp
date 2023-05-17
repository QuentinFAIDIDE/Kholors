#include "VuMeter.h"
#include "../../Config.h"
#include "../Section.h"
#include <string>

VuMeter::VuMeter(std::string t, std::string id) : dbValueLeft(0.0), dbValueRight(0.0), title(t), identifier(id)
{
}

void VuMeter::paint(juce::Graphics &g)
{
    auto bounds = g.getClipBounds();

    // abort if no space available
    if (bounds.getWidth() < (VUMETER_WIDTH))
    {
        return;
    }

    // draw the top title
    juce::Colour bg = juce::Colours::transparentBlack;
    drawSection(g, bounds, title, bg, true);

    auto boxesArea = zoomToInnerSection(bounds);

    // we then draw the core meter
    paintCoreMeter(g, boxesArea);
}

void VuMeter::mouseDrag(const juce::MouseEvent &event)
{
    // TODO
}

void VuMeter::setDbValue(float leftVal, float rightVal)
{
    dbValueLeft = leftVal;
    dbValueRight = rightVal;
}

juce::Rectangle<int> VuMeter::zoomToInnerSection(juce::Rectangle<int> bounds)
{
    // focus on the area without the title and margins
    auto boxesArea = bounds;
    boxesArea.removeFromTop(SECTION_TITLE_HEIGHT_SMALL);
    boxesArea.reduce(TOPBAR_WIDGETS_MARGINS, TOPBAR_WIDGETS_MARGINS);

    // what's the size of the remaining side parts ?
    int emptySidesWidth = (boxesArea.getWidth() - VUMETER_WIDTH) / 2;

    // remove the side areas
    boxesArea.reduce(emptySidesWidth, 0);
    return boxesArea;
}

void VuMeter::paintCoreMeter(juce::Graphics &g, juce::Rectangle<int> boxesArea)
{
    // we then draw the inside of the vumeter
    g.setColour(COLOR_BACKGROUND);
    g.fillRect(boxesArea.withWidth(boxesArea.getWidth() / 2));
    g.setColour(COLOR_TEXT_DARKER.withAlpha(0.5f));
    g.drawRect(boxesArea.withWidth(boxesArea.getWidth() / 2));

    // apply the vumeter inner margins
    boxesArea.reduce(VUMETER_OUTTER_PADDING, VUMETER_OUTTER_PADDING);

    // isolate the left area where the vu meter lives
    auto vuArea = boxesArea.withWidth(boxesArea.getWidth() / 2);
    // isolate the right area where the scale lives
    auto scaleArea = vuArea.withX(vuArea.getX() + vuArea.getWidth());

    drawScale(g, scaleArea);
    drawMeter(g, vuArea);
}

void VuMeter::drawScale(juce::Graphics &g, juce::Rectangle<int> area)
{
    // how many horizontal grading lines we can fit
    int noLines = std::abs(VUMETER_MIN_DB) / VUMETER_SCALE_DEFINITION;

    int baseLineLength = 2;
    int currentLineHeight = 0;

    g.setColour(COLOR_TEXT_DARKER);
    g.setFont(juce::Font(8));

    // the space between lines
    float lineSpacing = float(area.getHeight()) / float(noLines);

    for (int i = 0; i <= noLines; i++)
    {
        currentLineHeight = float(i) * lineSpacing;

        // draw the line
        int y = area.getY() + currentLineHeight;
        g.drawLine(area.getX(), y, area.getX() + baseLineLength, y, 0.5f);

        // check the last bit to detect pair iterations
        if ((i % 2) == 0)
        {
            g.setColour(COLOR_TEXT_DARKER);
        }
        else
        {
            g.setColour(COLOR_TEXT_DARKER.withAlpha(0.5f));
        }

        // draw the text
        juce::Rectangle<int> textArea(area.getX(), y - 5, area.getWidth(), 10);
        g.drawText("- " + std::to_string(int(i * VUMETER_SCALE_DEFINITION)) + " dB", textArea,
                   juce::Justification::centredLeft, false);
    }
}

void VuMeter::drawMeter(juce::Graphics &g, juce::Rectangle<int> area)
{
}