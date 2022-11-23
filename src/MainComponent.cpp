#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent() : arrangementArea(sampleManager, notificationArea) {
  setSize(800, 1422);
  // make it visible
  addAndMakeVisible(arrangementArea);
  addAndMakeVisible(notificationArea);
}

MainComponent::~MainComponent() {}

//==============================================================================
void MainComponent::paint(juce::Graphics&) {}

void MainComponent::resized() { arrangementArea.setBounds(getLocalBounds()); }