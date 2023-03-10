#include "Section.h"

Section::Section() {
  _title = "Section";
  _content = nullptr;
}

Section::Section(std::string title) : _title(title) { _content = nullptr; }

Section::~Section() {}

void Section::paint(juce::Graphics &g) {
  juce::Rectangle<int> bounds = g.getClipBounds();
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
  juce::Rectangle<int> bounds = getBounds();
  bounds.reduce(4, 4);
  if (_content != nullptr) {
    _content->setBounds(bounds);
  }
}

void Section::setContent(juce::Component *c) {
  _content = c;
  addAndMakeVisible(_content);
}