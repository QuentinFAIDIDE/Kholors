#include "MainComponent.h"

#include <cstdlib>
#include <memory>

#include "EmptyTab.h"
#include "RobotoFont.h"

//==============================================================================
MainComponent::MainComponent()
    : sampleManager(notificationArea),
      arrangementArea(sampleManager, notificationArea),
      actionTabs(juce::TabbedButtonBar::Orientation::TabsAtTop) {
  configureLookAndFeel();

  // initialize audio app with two outputs
  setAudioChannels(0, 2);

  // set size of the component
  setSize(1400, 800);

  actionTabs.addTab("Audio Library", juce::Colour(20, 20, 20), &audioLibraryTab,
                    false);
  actionTabs.addTab("Sample Processing", juce::Colour(20, 20, 20),
                    &sampleProcessingTab, false);
  actionTabs.addTab("Mastering", juce::Colour(20, 20, 20), &masteringTab,
                    false);

  // make subwidgets visible
  addAndMakeVisible(arrangementArea);
  addAndMakeVisible(notificationArea);
  addAndMakeVisible(actionTabs);

  // set the sampleManager callback to repaint arrangement area
  sampleManager.setTrackRepaintCallback([this] {
    const juce::MessageManagerLock mmLock;
    arrangementArea.repaint();
  });
}

MainComponent::~MainComponent() {
  setLookAndFeel(nullptr);
  // shutdown the audio
  shutdownAudio();
}

void MainComponent::configureLookAndFeel() {
  // create Roboto font
  juce::Typeface::Ptr tface = juce::Typeface::createSystemTypefaceFor(
      RobotoFont::RobotoRegular_ttf, RobotoFont::RobotoRegular_ttfSize);
  // set roboto as default font
  appLookAndFeel.setDefaultSansSerifTypeface(tface);
  setLookAndFeel(&appLookAndFeel);
}

//==============================================================================
void MainComponent::paint(juce::Graphics& g) {
  // draw background over bounds
  // paint the background of the area
  g.setColour(COLOR_APP_BACKGROUND);
  g.fillRect(g.getClipBounds());
}

void MainComponent::resized() {
  // get window coordinations
  juce::Rectangle<int> localBounds = getLocalBounds();
  // set notification area
  localBounds.setX(0);
  localBounds.setY(0);
  localBounds.setHeight(NOTIF_HEIGHT + NOTIF_OUTTER_MARGINS +
                        NOTIF_OUTTER_MARGINS);
  notificationArea.setBounds(localBounds);
  // set arrangement area to a FREQTIME_VIEW_HEIGHT band at
  // middle of the screen
  localBounds.setX(0);
  localBounds.setY(NOTIF_HEIGHT + NOTIF_OUTTER_MARGINS + NOTIF_OUTTER_MARGINS);
  localBounds.setHeight(FREQTIME_VIEW_HEIGHT);
  arrangementArea.setBounds(localBounds);

  int y = NOTIF_HEIGHT + NOTIF_OUTTER_MARGINS + NOTIF_OUTTER_MARGINS +
          FREQTIME_VIEW_HEIGHT + 6;
  localBounds = getLocalBounds();
  localBounds.setY(y);
  localBounds.setHeight(localBounds.getHeight() - y - 6);
  actionTabs.setBounds(localBounds);
}

void MainComponent::prepareToPlay(int samplesPerBlockExpected,
                                  double sampleRate) {
  // pass that callback down to the sample mananger
  sampleManager.prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void MainComponent::releaseResources() {
  // pass that callback down to the sample mananger
  sampleManager.releaseResources();
}

void MainComponent::getNextAudioBlock(
    const juce::AudioSourceChannelInfo& bufferToFill) {
  // pass that callback down to the sample mananger
  sampleManager.getNextAudioBlock(bufferToFill);
}

void MainComponent::configureApp(Config& conf) {
  audioLibraryTab.initAudioLibrary(conf);
}