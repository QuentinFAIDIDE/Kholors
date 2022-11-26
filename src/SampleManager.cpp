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

    // set master bus gain
    masterGain.setGainDecibels(0.0f);
    // make sure it's smoothing changes
    masterGain.setRampDurationSeconds(DSP_GAIN_SMOOTHING_RAMP_SEC);
    // warn if smoothing is disabled
    if (!masterGain.isSmoothing()) {
        std::cerr << "Unable to set master gain smoothing" << std::endl;
    }

    // set master limiter parameters
    masterLimiter.setThresold(-DSP_DEFAULT_MASTER_LIMITER_HEADROOM_DB);
    masterLimiter.setRelease(DSP_DEFAULT_MASTER_LIMITER_RELEASE_MS);
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
    // design problem: this can't fail as it's async :D 
    // so this return value is useless.
    // We will notify the notification service in the background thread.
    return 0;
}

void SampleManager::prepareToPlay(int samplesPerBlockExpected, double sampleRate) {

    // prepare all inputs
    for(size_t i = 0; i<tracks.size(); i++) {
        tracks.getUnchecked(i).prepareToPlay(samplesPerBlockExpected, sampleRate);
    }

    // prepare all master bus effects if necessary

    // for the limiter we need to fill a spec object
    currentAudioSpec.sampleRate = sampleRate;
    currentAudioSpec.maximumBlockSize = juce::uint32(samplesPerBlockExpected);
    currentAudioSpec.numChannels = juce::uint32(getTotalNumOutputChannels());

    // reset the limiter internal states
    masterLimiter.reset();
    // prepare to play with these settings
    masterLimiter.prepare(currentAudioSpec);

    // TODO: look at MixerAudioSource code to double check what is performed is ok
}

void SampleManager::releaseResources() {
    // TODO: look at MixerAudioSource code to double check what is performed is ok

    // call all inputs releaseResources
    for(size_t i = 0; i<tracks.size(); i++) {
        tracks.getUnchecked(i).releaseResources();
    }

    // reset the limiter internal states
    masterLimiter.reset();

    // clear output buffer
    audioThreadBuffer.setSize (2, 0);
}

void SampleManager::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) {
    // the mixing code here was initially based on the MixerAudioSource one from Juce.

    // get scoped lock of the reentering mutex
    // TODO: This lock is preventing race conditions on tracks and bitmask.
    // We don't load any yet but it's good to know !
    const ScopedLock sl (mixbusMutex);

    // Idea: make it so that we use the lsit of SamplePlayer
    // instead of the list of tracks.
    // Also make the sample list keep nullptr in the list 
    // isntead of deleting so that we can safely use ids.
    // After that, we can make the bitmask atomic and
    // try to make it so we need no mixbusMutex.

    // TODO: subset input tracks only to those that are
    // likely to play using a bitmask

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
        );

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

        // apply master bus gain
        bufferToFill.buffer.applyGain(masterGain);

        // apply limiting
        juce::dsp::ProcessContextReplacing<float> context(
            juce::dsp::AudioBlock<float>(bufferToFill.buffer)
        );
        masterLimiter.process(context);

        // we need to update the read cursor position
        positionM
    } else {
        // if there's no tracks, clear output
        info.clearActiveBufferRegion();
    }
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