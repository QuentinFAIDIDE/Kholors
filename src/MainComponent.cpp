#include "MainComponent.h"
#include "RobotoFont.h"

//==============================================================================
MainComponent::MainComponent() : arrangementArea(sampleManager, notificationArea) {
  // create Roboto font
  juce::Typeface::Ptr tface = juce::Typeface::createSystemTypefaceFor(
    RobotoFont::RobotoRegular_ttf,
    RobotoFont::RobotoRegular_ttfSize
  );
  juce::LookAndFeel::getDefaultLookAndFeel().setDefaultSansSerifTypeface (tface);

  setSize(800, 1422);
  // make it visible
  addAndMakeVisible(arrangementArea);
  addAndMakeVisible(notificationArea);
}

MainComponent::~MainComponent() {}

//==============================================================================
void MainComponent::paint(juce::Graphics& g) {
  // draw background over bounds
  // paint the background of the area
  g.setColour(COLOR_APP_BACKGROUND);
  g.fillRect(g.getClipBounds());
}

void MainComponent::resized() {
  // TODO: Hide notifications or make it an overlay if they overlap
  // get window coordinations
  juce::Rectangle<int> localBounds = getLocalBounds();
  // set arrangement area to a FREQTIME_VIEW_HEIGHT band at
  // middle of the screen
  localBounds.setX(0);
  localBounds.setY((localBounds.getHeight()-FREQTIME_VIEW_HEIGHT)>>1);
  localBounds.setHeight(FREQTIME_VIEW_HEIGHT);
  arrangementArea.setBounds(localBounds);
  // set notification area
  localBounds.setX(0);
  localBounds.setY(0);
  localBounds.setHeight(NOTIF_HEIGHT+NOTIF_OUTTER_MARGINS+NOTIF_OUTTER_MARGINS);
  notificationArea.setBounds(localBounds);
}