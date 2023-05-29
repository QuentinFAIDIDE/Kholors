#include "TempoGrid.h"

#include "../../Config.h"

#define BAR_LINE_HEIGHT 8
#define TEMPO_GRID_GRADIENT_HEIGHT 40
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
    juce::ColourGradient topGradient(juce::Colours::transparentBlack, halfBounds.getX(),
                                     halfBounds.getTopLeft().getY() - TEMPO_GRID_GRADIENT_HEIGHT, COLOR_BACKGROUND,
                                     halfBounds.getX(), halfBounds.getTopLeft().getY(), false);

    g.setGradientFill(topGradient);
    g.fillRect(halfBounds.getX(), halfBounds.getTopLeft().getY() - TEMPO_GRID_GRADIENT_HEIGHT, halfBounds.getWidth(),
               TEMPO_GRID_GRADIENT_HEIGHT);

    juce::ColourGradient botGradient(COLOR_BACKGROUND, halfBounds.getX(), halfBounds.getTopLeft().getY(),
                                     juce::Colours::transparentBlack, halfBounds.getX(),
                                     halfBounds.getTopLeft().getY() + TEMPO_GRID_GRADIENT_HEIGHT, false);

    g.setGradientFill(botGradient);
    g.fillRect(halfBounds.getX(), halfBounds.getTopLeft().getY(), halfBounds.getWidth(), TEMPO_GRID_GRADIENT_HEIGHT);

    // draw a horizontal line in the middle
    g.setColour(COLOR_TEXT_DARKER);
    g.drawLine(centerLine.toFloat(), 1);

    // now draw one tick at every bar
    float barPixelWidth = (60.0 * float(AUDIO_FRAMERATE)) / (tempo * viewScale);
    int noBars = float(bounds.getWidth()) / barPixelWidth;

    auto barTickLine = juce::Line<float>();
    auto barTickNumberArea = juce::Rectangle<float>();

    g.setFont(juce::Font(TEMPO_GRID_FONT_SIZE));

    for (int i = 0; i <= noBars; i++)
    {
        float xOrigin = bounds.getX() + float(i) * barPixelWidth;
        barTickLine.setStart(xOrigin, halfBounds.getY() - BAR_LINE_HEIGHT);
        barTickLine.setEnd(xOrigin, halfBounds.getY() + BAR_LINE_HEIGHT);
        g.drawLine(barTickLine, 2);

        barTickNumberArea.setX(xOrigin - (TEMPO_GRID_TEXT_WIDTH / 2));
        barTickNumberArea.setWidth(TEMPO_GRID_TEXT_WIDTH);
        barTickNumberArea.setY(halfBounds.getY() + TEMPO_GRID_TEXT_Y_OFFSET);
        barTickNumberArea.setHeight(TEMPO_GRID_TEXT_WIDTH);

        g.drawText("0", barTickNumberArea, juce::Justification::centredTop, false);
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