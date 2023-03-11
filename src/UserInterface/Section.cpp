#include "Section.h"

void drawSection(juce::Graphics &g, juce::Rectangle<int> &bounds,
                 juce::String title, juce::Colour &background) {
  g.setColour(background);
  g.fillRoundedRectangle(bounds.getX(), bounds.getY(), bounds.getWidth(),
                         bounds.getHeight(), 4);

  g.setColour(juce::Colour(220, 220, 220));
  g.drawRoundedRectangle(bounds.getX(), bounds.getY(), bounds.getWidth(),
                         bounds.getHeight(), 4, 0.4);

  g.setColour(juce::Colour::fromFloatRGBA(0, 0, 0, 0.1));
  g.fillRoundedRectangle(bounds.getX(), bounds.getY() + bounds.getHeight() - 6,
                         bounds.getWidth(), 6, 4);

  g.fillRoundedRectangle(bounds.getX() + bounds.getWidth() - 6,
                         bounds.getY() + 18, 6, bounds.getHeight() - 18 - 6, 4);

  g.setColour(juce::Colour::fromFloatRGBA(255, 255, 255, 0.03));
  g.fillRect(bounds.getX(), bounds.getY() + 18, 5, bounds.getHeight() - 18 - 5);

  g.setColour(juce::Colour::fromFloatRGBA(1, 1, 1, 0.1));
  g.fillRect(bounds.getX(), bounds.getY(), bounds.getWidth(), 18);
  g.drawRect(bounds.getX(), bounds.getY(), bounds.getWidth(), 18, 1);

  g.setColour(juce::Colour(220, 220, 220));
  g.drawText(title, bounds.getX(), bounds.getY(), bounds.getWidth(), 18,
             juce::Justification::centred, false);
}