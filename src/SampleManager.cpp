#include "SampleManager.h"

#include <iostream>

// initialize the SampleManager, as well as Thread and audio inherited
// behaviours.
SampleManager::SampleManager(NotificationArea& na) :
    notificationManager(na),
    playCursor(0),
    Thread("Background Thread")
{
  // initialize format manager
  formatManager.registerBasicFormats();

  // set 0 inputs and two outputs
  // TODO: make this a cli arg
  setAudioChannels(0, 2);

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
  stopThread(4000);
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
    const juce::ScopedLock lock(pathMutex);
    // swap the strings with the one to load
    filePathToImport.swapWith(filePath);
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

void SampleManager::prepareToPlay(int samplesPerBlockExpected,
                                  double sampleRate) {
  // prepare all inputs
  for (size_t i = 0; i < tracks.size(); i++) {
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
  for (size_t i = 0; i < tracks.size(); i++) {
    tracks.getUnchecked(i).releaseResources();
  }

  // reset the limiter internal states
  masterLimiter.reset();

  // clear output buffer
  audioThreadBuffer.setSize(2, 0);
}

void SampleManager::getNextAudioBlock(
    const juce::AudioSourceChannelInfo& bufferToFill) {
  // the mixing code here was initially based on the MixerAudioSource one from
  // Juce.

  // get scoped lock of the reentering mutex
  // TODO: This lock is preventing race conditions on tracks and bitmask.
  // We don't load any yet but it's good to know !
  const ScopedLock sl(mixbusMutex);

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
    juce::AudioSourceChannelInfo copyBufferDest(&audioThreadBuffer, 0,
                                                bufferToFill.numSamples);

    // for each input source
    for (size_t i = 1; i < tracks.size(); i++) {
      // get the next audio block in the buffer
      tracks.getUnchecked(i).getNextAudioBlock(copyBufferDest);
      // append it to the initial one
      for (int chan = 0; chan < bufferToFill.buffer->getNumChannels(); chan++) {
        bufferToFill.addFrom(chan, bufferToFill.startSample, audioThreadBuffer,
                             chan, 0, bufferToFill.numSamples)
      }
    }

    // apply master bus gain
    bufferToFill.buffer.applyGain(masterGain);

    // apply limiting
    juce::dsp::ProcessContextReplacing<float> context(
        juce::dsp::AudioBlock<float>(bufferToFill.buffer));
    masterLimiter.process(context);

    // we need to update the read cursor position with the samples
    // we've sent.
    playCursor += bufferToFill.numSamples();
  } else {
    // if there's no tracks, clear output
    info.clearActiveBufferRegion();
  }
}

// background thread content for allocating stuff
void SampleManager::run() {
  // wait 500ms in a loop unless notify is called
  while (!threadShouldExit()) {
    // update the playing SamplePlayer/tracks bitmask
    updateNearbySamplesBitmask();
    // check for files to import
    checkForFileToImport();
    // check for buffers to free
    checkForBuffersToFree();
    wait(500);
  }
}

void SampleManager::checkForBuffersToFree() {
  // inspired by LoopingAudioSampleBuffer juce tutorial

  // reminder: no need to lock for buffers reads here since no
  // write happens outside of the current
  // background thread (that launches this procedure)

  // for each buffer where we store audio samples for
  for (auto i = buffers.size(); --i >= 0;) {
    // get it
    ReferenceCountedBuffer::Ptr buffer(buffers.getUnchecked(i));
    // if it's only referenced here and in the list
    if (buffer->getReferenceCount() == 2)
      // free it
      buffers.remove(i);
  }
}

void SampleManager::checkForFileToImport() {
  // inspired by LoopingAudioSampleBuffer juce tutorial

  // desired position for sample to import
  int64_t desiredPosition;

  // swap class member string of path to open with an empty one
  juce::String pathToOpen;
  {
    const juce::ScopedLock lock(pathMutex);
    pathToOpen.swapWith(filePathToImport);
    // also copy the position at the same moment
    desiredPosition = filePositionToImport;
  }

  // if the path is non empty (meaning we're awaiting importing)
  if (pathToOpen.isNotEmpty()) {
    // create file object
    juce::File file(pathToOpen);
    // create reader object
    std::unique_ptr<juce::AudioFormatReader> reader(
        formatManager.createReaderFor(file));
    // if the reader can read our file
    if (reader.get() != nullptr) {
      // sample duration in seconds
      auto duration = (float)reader->lengthInSamples / reader->sampleRate;

      // only load it if below our global constant maximum sample duration
      if (duration < SAMPLE_MAX_DURATION_SEC) {

        // the new buffer reference
        ReferenceCountedBuffer::Ptr newBuffer = new ReferenceCountedBuffer(
            file.getFileName(),
            (int)reader->numChannels,
            (int)reader->lengthInSamples
        );

        // do the actual reading
        reader->read(newBuffer->getAudioSampleBuffer(), 0,
                     (int)reader->lengthInSamples, 0, true, true);

        // TODO: create a sample instance that points to this buffer
        {
          // this part has to be replaced by our custom logic
          const juce::SpinLock::ScopedLockType lock(mutex);
          currentBuffer = newBuffer;
          buffers.add(newBuffer);
        }

      } else {
        // notify user about sample being too long to be loaded
        notificationManager.notify(
            juce::String("Due to a hardcoded sample duration limit of ")+
            juce::String(SAMPLE_MAX_DURATION_SEC)+
            juce::String(" seconds, this sample was not loaded: ")+
            pathToOpen
        );
      }
    } else {
        // if the file reader doesn't want to read, notify an error
        notificationManager.notify(juce::String("Unable to read: ")+pathToOpen);
    }
  }
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

void sampleManager::updateNearbySamplesBitmask() {
  // TODO
}