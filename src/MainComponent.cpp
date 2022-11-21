#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
    setSize (800, 1422);
    // make it visible
    addAndMakeVisible(arrangementArea);
}

MainComponent::~MainComponent()
{
    
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{

}

void MainComponent::resized()
{
    arrangementArea.setBounds(getLocalBounds());
}