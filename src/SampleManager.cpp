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
    // the mixing code here was initially based on MixerAudioSource one.
    
    // get scoped lock of the reentering mutex
    const ScopedLock sl (mixbusMutex);

    // TODO: subset input tracks only to those that are
    // likely to play

    // if there is more then one input track
    if (tracks.size() > 0) {
        // get a pointer to a new processed input buffer from first source
        // we will append into this one to mix tracks together
        tracks.getUnchecked(0).getNextAudioBlock(bufferToFill);

        // create a new getNextAudioBlock request that 
        // will use our SampleManager buffer to pull
        // block to append to the previous buffer
        juce::AudioSourceChannelInfo copyBufferDest(
            &audioThreadBuffer,
            0,
            bufferToFill.numSamples
        )

        // for each input source
        for(size_t i = 1; i<tracks.size(); i++) {
            // get the next audio block in the buffer
            tracks.getUnchecked(i).getNextAudioBlock(copyBufferDest);
            // append it to the initial one
            for (int chan = 0; chan < bufferToFill.buffer->getNumChannels(); chan++) {
                bufferToFill.addFrom(
                    chan,
                    bufferToFill.startSample,
                    audioThreadBuffer,
                    chan,
                    0,
                    bufferToFill.numSamples    
                )
            }
        }

        // we need to update the read cursor position

    }

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
    // TODO: see LoopingAudioSampleBuffer tutorial example
}

void SampleManager::checkForFileToImport() {
    // TODO: see LoopingAudioSampleBuffer tutorial example
}

void SampleManager::setNextReadPosition(int64) {
    // TODO
}

int64 SampleManager::getNextReadPosition(int64) {
    // TODO
}

int64 SampleManager::getTotalLength() {
    // TODO
}

bool SampleManager::isLooping() {
    // TODO
}

void SampleManager::setLooping(bool) {
    // TODO
}