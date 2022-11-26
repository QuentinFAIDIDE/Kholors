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
    // TODO: look at MixerAudioSource code to better understand it
    // TODO: call all inputs prepareToPlay
}

void SampleManager::releaseResources() {
    // TODO: look at MixerAudioSource code to better understand it
    // TODO: call all inputs releaseResources

    // clear output
}

void SampleManager::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) {
    // get scoped lock of reentering mutex

    // if there is more then one input track
        // get a pointer to a new processed input buffer from first source
        // we will append into this one to mix tracks together

        // create a new getNextAudioBlock request that 
        // will use our SampleManager buffer to pull
        // block to append to the previous buffer

        // for each input source
            // get the next audio block in the buffer
            // append it to the initial one

    // if there's no tracks, clear output
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