#include "SampleManager.h"

#include <iostream>

// initialize the SampleManager, as well as Thread and audio inherited
// behaviours.
SampleManager::SampleManager(NotificationArea& na)
    : notificationManager(na),
      playCursor(0),
      Thread("Background Thread"),
      totalFrameLength(0),
      numChannels(2),
      isPlaying(false) {
  // initialize format manager
  formatManager.registerBasicFormats();

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
  masterLimiter.setThreshold(-DSP_DEFAULT_MASTER_LIMITER_HEADROOM_DB);
  masterLimiter.setRelease(DSP_DEFAULT_MASTER_LIMITER_RELEASE_MS);

  // allocate bitmask for tracks
  nearTracksBitmask = new int64_t[SAMPLE_BITMASK_SIZE];
  backgroundNearTrackBitmask = new int64_t[SAMPLE_BITMASK_SIZE];

  // set the nearby samplePlayer/tracks bitmask to 0
  for (size_t i = 0; i < SAMPLE_BITMASK_SIZE; i++) {
    nearTracksBitmask[i] = 0;
    backgroundNearTrackBitmask[i] = 0;
  }
}

SampleManager::~SampleManager() {
  // stop thread with a 4sec timeout to kill it
  stopThread(4000);
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
    tracks.getUnchecked(i)->prepareToPlay(
      samplesPerBlockExpected,
      sampleRate
    );
  }

  // prepare all master bus effects if necessary

  // for the limiter we need to fill a spec object
  currentAudioSpec.sampleRate = sampleRate;
  currentAudioSpec.maximumBlockSize = juce::uint32(samplesPerBlockExpected);
  currentAudioSpec.numChannels = numChannels;

  // reset the limiter internal states
  masterLimiter.reset();
  // prepare to play with these settings
  masterLimiter.prepare(currentAudioSpec);

  // recomputes nearby tracks bitmask
  // and allocate/free memory around in the background thread
  notify();

  // TODO: look at MixerAudioSource code to double check what is performed is ok
}

void SampleManager::releaseResources() {
  // TODO: look at MixerAudioSource code to double check what is performed is ok

  // call all inputs releaseResources
  for (size_t i = 0; i < tracks.size(); i++) {
    tracks.getUnchecked(i)->releaseResources();
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
  const juce::ScopedLock sl(mixbusMutex);

  // TODO: subset input tracks only to those that are
  // likely to play using the bitmask

  // if there is more then one input track
  if (tracks.size() > 0) {
    // get a pointer to a new processed input buffer from first source
    // we will append into this one to mix tracks together
    tracks.getUnchecked(0)->getNextAudioBlock(bufferToFill);

    // create a new getNextAudioBlock request that
    // will use our SampleManager buffer to pull
    // block to append to the previous buffer
    juce::AudioSourceChannelInfo copyBufferDest(&audioThreadBuffer, 0,
                                                bufferToFill.numSamples);

    // for each input source
    for (size_t i = 1; i < tracks.size(); i++) {
      // get the next audio block in the buffer
      tracks.getUnchecked(i)->getNextAudioBlock(copyBufferDest);
      // append it to the initial one
      for (int chan = 0; chan < bufferToFill.buffer->getNumChannels(); chan++) {
        bufferToFill.buffer->addFrom(chan, bufferToFill.startSample, audioThreadBuffer,
                             chan, 0, bufferToFill.numSamples);
      }
    }

    // create context to apply dsp effects

    juce::dsp::AudioBlock<float> audioBlockRef(
      *bufferToFill.buffer,
      bufferToFill.startSample
    );
    juce::dsp::ProcessContextReplacing<float> context(audioBlockRef);
    
    // apply master gain
    masterGain.process(context);

    // apply limiter
    masterLimiter.process(context);

    // we need to update the read cursor position with the samples
    // we've sent.
    playCursor += bufferToFill.numSamples;
  } else {
    // if there's no tracks, clear output
    bufferToFill.clearActiveBufferRegion();
  }
}

// background thread content for allocating stuff
void SampleManager::run() {
  // wait 500ms in a loop unless notify is called
  while (!threadShouldExit()) {
    // check for files to import
    checkForFileToImport();
    // check for buffers to free
    checkForBuffersToFree();
    // do we need to stop playback because the cursor is not in bounds ?
    pauseIfCursorNotInBound();
    // update the playing SamplePlayer/tracks bitmask
    updateNearbySamplesBitmask();
    // wait untill next thread iteration
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
    BufferPtr buffer(buffers.getUnchecked(i));
    // if it's only referenced here and in the buffer list
    // meaning it's not used in a SamplePlayer
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
        BufferPtr newBuffer = new ReferenceCountedBuffer(
            file.getFileName(), (int)reader->numChannels,
            (int)reader->lengthInSamples);

        // do the actual reading
        reader->read(newBuffer->getAudioSampleBuffer(), 0,
                     (int)reader->lengthInSamples, 0, true, true);

        // TODO: compute hash of sample, and do not save those
        // which already exists. Use a function like getBufferFromHash
        // to set the new samplePlayer to the already existing sample.

        // create a new sample player
        SamplePlayer* newSample = new SamplePlayer(desiredPosition);

        // get a scoped lock for the buffer array
        {
          const juce::SpinLock::ScopedLockType lock(mutex);

          // assign buffer to the new SamplePlayer
          newSample->setBuffer(newBuffer);

          // add new SamplePlayer to the tracks list (positionable audio
          // sources)
          tracks.add(newSample);

          // add the buffer the array of audio buffers
          buffers.add(newBuffer);
        }

      } else {
        // notify user about sample being too long to be loaded
        notificationManager.notifyError(
            juce::String("Due to a hardcoded sample duration limit of ") +
            juce::String(SAMPLE_MAX_DURATION_SEC) +
            juce::String(" seconds, this sample was not loaded: ") +
            pathToOpen);
      }
    } else {
      // if the file reader doesn't want to read, notify an error
      notificationManager.notifyError(juce::String("Unable to read: ") + pathToOpen);
    }
  }
}

void SampleManager::setNextReadPosition(juce::int64 nextReadPosition) {
  // here we update the nearby sample bitmask and update their position

  // TODO: look at and fix eventual audio glitches

  // update play cursor
  playCursor = nextReadPosition;

  // tell all samplePlayers to update positions
  for (size_t i = 0; i < tracks.size(); i++) {
    // tell the track to reconsider its position
    tracks.getUnchecked(i)->setNextReadPosition(nextReadPosition);
  }
  
  // update playing tracks bitmask
  updateNearbySamplesBitmask();

  // tells the background thread to update position
  notify();
}

juce::int64 SampleManager::getNextReadPosition() const {
  // TODO: simply returns total frame position
  return playCursor;
}

juce::int64 SampleManager::getTotalLength() const {
  // TODO: simply returns total length of the entire track in frames
  return totalFrameLength;
}

bool SampleManager::isLooping() const {
  // TODO: always return false, we don't allow looping the whole track
  //       maybe this can be a feature for later. We could also loop only
  //       between some loop marks in the future, even though it look like
  //       it's not the intended use for this callback.
  return false;
}

void SampleManager::updateNearbySamplesBitmask() {
  // clear the upcoming bitmask we will swap with the one audio
  // thread uses
  for (size_t i = 0; i < SAMPLE_BITMASK_SIZE; i++) {
    // no need to clear above the last track
    if (i > 0 && ((i - 1) * 64) > tracks.size()) {
      break;
    }
    backgroundNearTrackBitmask[i] = 0;
  }

  // In the following algo the approach is O(n) but it
  // could be O(log(n)) or less.

  // TODO: use an ordered list of SamplePlayer list ids to speed up
  // the nearby track masking process.
  // We can leverage the fact that we have a maximum SamplePlayer/buffer length
  // to set the bits at all the ids after first than interpolates
  // playCursor with BASE_POSITION + MAX_SAMPLE_SIZE and the first one that
  // doesn't after that.
  // such an ordered list can be search in log(n) time by looking at sucessive
  // halvings from the middle track.

  // less effective but simpler implementation:

  int64_t rangeBuffer, baseRow;

  // for each block of 64 bits of the nearby tracks bitmask
  for (size_t i = 0; i < SAMPLE_BITMASK_SIZE; i++) {
    // a buffer for the range of bits
    rangeBuffer = 0;

    // avoid repeating the multiplication
    baseRow = i * 64;

    // for each bit representing an individual track in the range
    for (size_t j = 0; j < 64 && baseRow + j < tracks.size(); j++) {
      // if the corresponding track is SAMPLE_MASKING_DISTANCE audio
      // frames close to the playing cursor
      if (tracks.getUnchecked(baseRow + j)->getNextReadPosition() >
              playCursor - SAMPLE_MASKING_DISTANCE_FRAMES &&
          tracks.getUnchecked(baseRow + j)->getNextReadPosition() <
              playCursor + SAMPLE_MASKING_DISTANCE_FRAMES) {
        // set the bit
        rangeBuffer = rangeBuffer | (1 << (j - 63));
      }
    }
    // write the bit range
    backgroundNearTrackBitmask[i] = rangeBuffer;

    // abort if we're out of bounds
    if (baseRow + 63 >= tracks.size()) {
      break;
    }
  }

  // pointer to swap arrays
  int64_t* swapBuffer;

  // get lock and swap the bitmasks
  {
    const juce::SpinLock::ScopedLockType lock(mutex);

    // add the buffer the array of audio buffers
    swapBuffer = nearTracksBitmask;
    nearTracksBitmask = backgroundNearTrackBitmask;
    backgroundNearTrackBitmask = swapBuffer;
  }
}

void SampleManager::pauseIfCursorNotInBound() {
  // TODO
}

