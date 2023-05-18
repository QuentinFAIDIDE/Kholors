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
    g.fillRect(boxesArea.withWidth((boxesArea.getWidth() / 2) + 2));

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
    // isolate left and right channels
    auto leftMeter = area.withWidth(area.getWidth() / 2);
    auto rightMeter = leftMeter.withX(leftMeter.getX() + leftMeter.getWidth());

    // remove the sides
    leftMeter.reduce(VUMETER_INNER_PADDING, 0);
    rightMeter.reduce(VUMETER_INNER_PADDING, 0);

    drawChannel(g, leftMeter, dbValueLeft);
    drawChannel(g, rightMeter, dbValueRight);
}

void VuMeter::drawChannel(juce::Graphics &g, juce::Rectangle<int> area, float value)
{
    // ideal rectangle length
    int rectHeigth = float(area.getHeight()) / float(VUMETER_DEFINITION);

    // This is the effective resolution (number of levels) that can be different
    // from requested one whenever int truncating make it so that we can fit one more.
    // This is a lazy solution I admit but it's straightforward.
    int effectiveResolution = area.getHeight() / rectHeigth;

    // compute remaining area at top (because of int truncating)
    int remainingPixels = area.getHeight() - (effectiveResolution * rectHeigth);
    for (int i = 0; i < effectiveResolution; i++)
    {
        juce::Rectangle<int> rectToDraw(area.getX(),
                                        area.getY() + area.getHeight() - (rectHeigth * (i + 1)) - (remainingPixels / 2),
                                        area.getWidth(), rectHeigth);
        rectToDraw.removeFromTop(VUMETER_INNER_PADDING);

        juce::Colour color = pickColorForIndex(i, effectiveResolution, value);

        g.setColour(color);

        g.fillRect(rectToDraw);
    }
}

juce::Colour VuMeter::pickColorForIndex(int index, int maxIndex, float dbVolume)
{
    int dbMeterPosition = juce::jmap((float)index / (float)(maxIndex - 1), VUMETER_MIN_DB, 0.0f);

    juce::Colour col;
    if (dbMeterPosition >= VUMETER_COLOR_3_MAX_DB)
    {
        col = COLOR_VUMETER_3;
    }
    else if (dbMeterPosition >= VUMETER_COLOR_2_MAX_DB)
    {
        col = COLOR_VUMETER_2;
    }
    else if (dbMeterPosition >= VUMETER_COLOR_1_MAX_DB)
    {
        col = COLOR_VUMETER_1;
    }
    else
    {
        col = COLOR_VUMETER_0;
    }

    if (dbMeterPosition >= dbVolume)
    {
        col = col.withAlpha(0.5f);
    }

    return col;
}