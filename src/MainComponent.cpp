#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent() : arrangementArea(sampleManager) {
  setSize(800, 1422);
  // make it visible
  addAndMakeVisible(arrangementArea);
}

MainComponent::~MainComponent() {}

//==============================================================================
void MainComponent::paint(juce::Graphics&) {}

void MainComponent::resized() { arrangementArea.setBounds(getLocalBounds()); }