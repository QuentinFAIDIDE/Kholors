#include "Section.h"

Section::Section() {
  _title = "Section";
  _content = nullptr;
}

Section::Section(std::string title) : _title(title) { _content = nullptr; }

Section::~Section() {}

void Section::paint(juce::Graphics &g) {
  g.setColour(juce::Colour(20, 20, 20));
  g.fillAll();

  juce::Rectangle<int> bounds = g.getClipBounds();

  if (bounds.getHeight() < 40 || bounds.getWidth() < 40) {
    return;
  }

  bounds.reduce(4, 4);

  // make space to draw title
  bounds.setY(bounds.getY() + 8);
  bounds.setHeight(bounds.getHeight() - 8);

  g.setColour(juce::Colour(20, 20, 20));
  g.fillRoundedRectangle(bounds.getX(), bounds.getY(), bounds.getWidth(),
                         bounds.getHeight(), 4);
  g.setColour(juce::Colour(230, 230, 230));
  g.drawRoundedRectangle(bounds.getX(), bounds.getY(), bounds.getWidth(),
                         bounds.getHeight(), 4, 1.0f);

  bounds.setX(bounds.getX() + 6);

  g.setColour(juce::Colour(20, 20, 20));
  g.fillRect(bounds.getX() + 3, bounds.getY() - 9,
             juce::Font(15.0f).getStringWidth(_title) + 8, 18);

  g.setColour(juce::Colour(200, 200, 200));
  g.setFont(15.0f);
  juce::Rectangle<int> textArea(bounds.getX() + 8, bounds.getY() - 7,
                                bounds.getWidth(), bounds.getHeight());
  g.drawText(_title, textArea, juce::Justification::topLeft, false);
}

void Section::resized() {
  if (_content != nullptr) {
    _content->setBounds(30, 30, getWidth() - 30, getHeight() - 30);
  }
}

void Section::setContent(juce::Component *c) {
  _content = c;
  addAndMakeVisible(*_content);
}