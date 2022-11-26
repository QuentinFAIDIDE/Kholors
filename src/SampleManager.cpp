#include "SampleManager.h"
#include <iostream>

// initialize the SampleManager, as well as Thread and audio inherited
// behaviours.
SampleManager::SampleManager():
    playCursor (0),
    Thread ("Background Thread")
{
    // initialize format manager
    formatManager.registerBasicFormats();

    // set 0 inputs and two outputs
    // TODO: make this a cli arg
    setAudioChannels (0, 2);

    // start the background thread that makes malloc/frees
    startThread();
}

SampleManager::~SampleManager() {
    // stop thread with a 4sec timeout to kill it
    stopThread (4000);
    // shutdown the audio
    shutdownAudio();
}

bool SampleManager::filePathsValid(const juce::StringArray& files) {
    // TODO: implement this
    return true;
}

int SampleManager::addSample(juce::String filePath, int64_t frameIndex) {
    // swap a file to load variable to avoid blocking audio thread for disk i/o
    {
        // get lock for scoped block
        const juce::ScopedLock lock (pathMutex);
        // swap the strings with the one to load
        filePathToImport.swapWith (filePath);
        // record the desired frame position to insert at
        filePositionToImport = frameIndex;
    }
    // notify the thread so it's triggered
    notify();
    return 0;
}

void SampleManager::prepareToPlay(int a, double b) {
    // TODO: find out how to use this properly
}

void SampleManager::releaseResources() {
    // TODO: document myself better on when this is called
    // TODO: get main lock
    // TODO: invalidate pointers so they can be freed
}

void SampleManager::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) {
    // TODO: get the block from MixerAudioSource ?
}

// background thread content for allocating stuff
void SampleManager::run() {
    // wait 500ms in a loop unless notify is called
    while(! threadShouldExit()) {
        // check for files to import
        checkForFileToImport();
        // check for buffers to free
        checkForBuffersToFree();
        wait(500);
    }
}

void SampleManager::checkForBuffersToFree() {
    // TODO
}

void SampleManager::checkForFileToImport() {
    // TODO
}