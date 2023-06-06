#include "TempoGrid.h"

#include "../../Config.h"

#define BAR_LINE_HEIGHT 8
#define SUBBAR_LINE_HEIGHT 4
#define TEMPO_GRID_GRADIENT_HEIGHT 64
#define TEMPO_GRID_TEXT_WIDTH 64
#define TEMPO_GRID_TEXT_X_OFFSET 0
#define TEMPO_GRID_TEXT_Y_OFFSET -24
#define TEMPO_GRID_FONT_SIZE 12

TempoGrid::TempoGrid()
{
    setInterceptsMouseClicks(false, false);
}

void TempoGrid::paint(juce::Graphics &g)
{
    auto bounds = g.getClipBounds();

    // lazy way to get a center line
    auto halfBounds = bounds.withY(bounds.getCentreY());
    auto centerLine = juce::Line<int>(halfBounds.getX(), halfBounds.getTopLeft().getY(),
                                      halfBounds.getX() + halfBounds.getWidth(), halfBounds.getTopLeft().getY());

    // draw a gradient in the middle
    juce::ColourGradient topGradient(
        juce::Colours::transparentBlack, halfBounds.getX(), halfBounds.getTopLeft().getY() - TEMPO_GRID_GRADIENT_HEIGHT,
        COLOR_BACKGROUND.withAlpha(0.5f), halfBounds.getX(), halfBounds.getTopLeft().getY(), false);

    g.setGradientFill(topGradient);
    g.fillRect(halfBounds.getX(), halfBounds.getTopLeft().getY() - TEMPO_GRID_GRADIENT_HEIGHT, halfBounds.getWidth(),
               TEMPO_GRID_GRADIENT_HEIGHT);

    juce::ColourGradient botGradient(COLOR_BACKGROUND.withAlpha(0.5f), halfBounds.getX(),
                                     halfBounds.getTopLeft().getY(), juce::Colours::transparentBlack, halfBounds.getX(),
                                     halfBounds.getTopLeft().getY() + TEMPO_GRID_GRADIENT_HEIGHT, false);

    g.setGradientFill(botGradient);
    g.fillRect(halfBounds.getX(), halfBounds.getTopLeft().getY(), halfBounds.getWidth(), TEMPO_GRID_GRADIENT_HEIGHT);

    // draw a horizontal line in the middle
    g.setColour(COLOR_TEXT_DARKER);
    g.drawLine(centerLine.toFloat(), 1);

    // now draw one tick at every bar

    // width of a tempo bar in audio frame
    float barFrameWidth = float(AUDIO_FRAMERATE * 60) / tempo;

    // width of a tempo bar in pixels
    float barPixelWidth = barFrameWidth / viewScale;

    // what's the position of the view in bars
    float viewStartBarIndex = viewPosition / barFrameWidth;

    float barPixelShift = barPixelWidth * (1.0f - (viewStartBarIndex - std::floor(viewStartBarIndex)));

    int firstDisplayedBar = ((int)viewStartBarIndex) + 1;

    int noBars = float(bounds.getWidth()) / barPixelWidth;

    auto barTickLine = juce::Line<float>();
    auto barTickNumberArea = juce::Rectangle<float>();

    g.setFont(juce::Font(TEMPO_GRID_FONT_SIZE));

    // now we prepare to draw the 4x4 sub tempo bars
    float subBarPixelWidth = barPixelWidth / 4.0f;
    float subBarFrameWidth = barFrameWidth / 4.0f;
    float subBarViewIndex = viewPosition / subBarFrameWidth;
    float subBarPixelShift = subBarPixelWidth * (1.0f - (subBarViewIndex - std::floor(subBarViewIndex)));

    for (int i = 0; i <= noBars * 4; i++)
    {

        if ((viewStartBarIndex * 4) + i < 4)
        {
            continue;
        }

        float xOrigin = bounds.getX() + subBarPixelShift + float(i) * subBarPixelWidth;
        barTickLine.setStart(xOrigin, halfBounds.getY() - SUBBAR_LINE_HEIGHT);
        barTickLine.setEnd(xOrigin, halfBounds.getY() + SUBBAR_LINE_HEIGHT);
        g.drawLine(barTickLine, 2);
    }

    float barWidth;
    float barHighlight;
    float eightBarsQueue;

    for (int i = 0; i <= noBars; i++)
    {
        eightBarsQueue = 0;

        if ((firstDisplayedBar + i) % 8 == 1)
        {
            g.setColour(COLOR_LABELS_BORDER);
            barWidth = 3;
            barHighlight = 3;
        }
        else
        {
            g.setColour(COLOR_TEXT_DARKER);
            barWidth = 2;
            barHighlight = 0;
        }

        if ((firstDisplayedBar + i) % (8 * 4) == 1)
        {
            g.setColour(juce::Colour(210, 170, 170));
            eightBarsQueue = 4;
        }

        float xOrigin = bounds.getX() + barPixelShift + float(i) * barPixelWidth;
        barTickLine.setStart(xOrigin, halfBounds.getY() - BAR_LINE_HEIGHT - barHighlight);
        barTickLine.setEnd(xOrigin, halfBounds.getY() + BAR_LINE_HEIGHT + barHighlight + eightBarsQueue);
        g.drawLine(barTickLine, barWidth);

        barTickNumberArea.setX(xOrigin - (TEMPO_GRID_TEXT_WIDTH / 2));
        barTickNumberArea.setWidth(TEMPO_GRID_TEXT_WIDTH);
        barTickNumberArea.setY(halfBounds.getY() + TEMPO_GRID_TEXT_Y_OFFSET - barHighlight);
        barTickNumberArea.setHeight(TEMPO_GRID_TEXT_WIDTH);

        g.setColour(COLOR_TEXT_DARKER);
        g.drawText(std::to_string(firstDisplayedBar + i), barTickNumberArea, juce::Justification::centredTop, false);
    }
}

void TempoGrid::updateTempo(float newTempo)
{
    tempo = newTempo;
}

void TempoGrid::updateView(int vp, float vs)
{
    viewPosition = vp;
    viewScale = vs;
}