#include "FrequencyGrid.h"
#include "../../Audio/UnitConverter.h"
#include <string>

std::vector<float> FrequencyGrid::displayedFreqs = {0.0, 150.0, 1000.0, 10000.0};

FrequencyGrid::FrequencyGrid()
{
    setInterceptsMouseClicks(false, false);
}

void FrequencyGrid::paint(juce::Graphics &g)
{

    // draw a dark gradient background to stress labels
    juce::ColourGradient leftGradient(COLOR_BACKGROUND, g.getClipBounds().getX() + FREQ_GRID_GRADIENT_SHIFT,
                                      g.getClipBounds().getTopLeft().getY(), COLOR_BACKGROUND.withAlpha(0.0f),
                                      g.getClipBounds().getX() + FREQ_GRID_GRADIENT_SHIFT + FREQ_GRID_GRADIENT_WIDTH,
                                      g.getClipBounds().getTopLeft().getY(), false);
    g.setGradientFill(leftGradient);
    g.fillRect(g.getClipBounds().removeFromLeft(FREQ_GRID_GRADIENT_WIDTH + FREQ_GRID_GRADIENT_SHIFT));

    // the line to be moved and drawn around
    juce::Line<float> line(g.getClipBounds().toFloat().getTopLeft(), g.getClipBounds().toFloat().getTopRight());

    // the text bounds to be drawn around
    juce::Rectangle<float> textArea =
        g.getClipBounds().toFloat().removeFromTop(FREQ_GRID_LABEL_HEIGHT).reduced(FREQ_GRID_LABEL_X_PADDING, 0);

    // the height in pixels of an onscreen left/right channel
    float channelHeight = g.getClipBounds().getHeight() >> 1;

    // the y position of the line
    float lineY;

    g.setFont(juce::Font(10));

    for (int i = 0; i < (int)displayedFreqs.size(); i++)
    {
        float positionRatio = UnitConverter::freqToPositionRatio(displayedFreqs[(size_t)i]);
        lineY = (1.0f - positionRatio) * channelHeight;

        // draw the top channel line
        line.setStart(line.getStart().withY(lineY));
        line.setEnd(line.getEnd().withY(lineY));
        g.setColour(COLOR_TEXT.withAlpha(0.5f));
        g.drawLine(line, FREQ_GRID_LINE_WIDTH);
        // draw the top channel text
        textArea.setY(lineY - FREQ_GRID_LABEL_HEIGHT);
        g.drawText(std::to_string(int(displayedFreqs[i])) + " Hz", textArea, juce::Justification::centredLeft, false);

        // now we draw line for the other channel below
        lineY = channelHeight * (1.0f + positionRatio);
        line.setStart(line.getStart().withY(lineY));
        line.setEnd(line.getEnd().withY(lineY));
        g.drawLine(line, FREQ_GRID_LINE_WIDTH);
        // and finally we draw the text if we're not at 0Hz
        if (i != 0)
        {
            textArea.setY(lineY - FREQ_GRID_LABEL_HEIGHT);
            g.drawText(std::to_string(int(displayedFreqs[i])) + " Hz", textArea, juce::Justification::centredLeft,
                       false);
        }
    }
}