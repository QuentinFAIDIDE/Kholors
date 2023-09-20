#ifndef DEF_FFT_RUNNER_HPP
#define DEF_FFT_RUNNER_HPP

#include <condition_variable>
#include <juce_audio_basics/juce_audio_basics.h>
#include <memory>
#include <mutex>
#include <vector>

#include "../WaitGroup.h"

// a cool post about C++ thread pools: https://stackoverflow.com/a/32593825

/**< Number of preallocated structure to pass FFT response data. 8192 is approximately 130Mb and rougthly 80 seconds of
 * stereo signal (# is linear to Mb) */
#define FFT_PREALLOCATED_JOB_STRUCTS 4096

/**< How many jobs we will sent together. Beware to keep number of job posting threads (not fft thread pool) below
 * FFT_PREALLOCATED_JOB_STRUCTS / FFT_JOBS_BATCH_SIZE */
#define FFT_JOBS_BATCH_SIZE 512

/**< How much zeros we pad at the end of fft input intensities for each intensity sample */
#define FFT_ZERO_PADDING_FACTOR 4

/**< Number of intensities we send as input (not accounting for zero padding after it). */
#define FFT_INPUT_NO_INTENSITIES 1024 // always choose a power of two!

/***< What is the overlap of subsequent FFT windows. 2 = 50% overlap, 3 = 66.666% overlap, 4=25% ... */
#define FFT_OVERLAP_DIVISION 2

/**< Size of the output, as the number of frequencies bins */
#define FFT_OUTPUT_NO_FREQS ((FFT_INPUT_NO_INTENSITIES * FFT_ZERO_PADDING_FACTOR) >> 1)

/**< jobs that are posted in the job queue and picked by threads  */
struct FftRunnerJob
{
    const float *input; /**< Audio intensities as inputs. Must be readable up to input+(sizeof(float)*inputLength) */
    float output[FFT_OUTPUT_NO_FREQS]; /**< frequency bins as output */
    float inputLength;                 /**< how many samples in the input are to be picked (from start) */
    std::shared_ptr<WaitGroup> wg;     /**< Synchronisation utility for batch of jobs across threads */
    std::mutex jobMutex; /**< To ensure atomic bools are synced (yeah wtf c++...) and we don't exit the waitgroup before
                            the other thread can read their right value */
    std::atomic<bool> resultFetched; /**< Has the result been fetched already ? */
};

/**
 * @brief Class that performs fft, eventually distributing
 *        computing between worker threads. It is meant to be used
 *        as a global static instance through juce::SharedRessourcePointer.
 */

class FftRunner
{
  public:
    /**
     * @brief Construct a new Fft Runner object
     *
     */
    FftRunner();

    /**
     * @brief Destroy the Fft Runner object
     *
     */
    ~FftRunner();

    /**
     * @brief Returns how many fft are covering an audio file
     *        with that much samples.
     *
     * @param numSamples The number of samples an audio files haves.
     * @return int The number of FFTs that will be returned for an audio file with that size.
     */
    static int getNumFftFromNumSamples(int numSamples);

    /**
     * @brief Perfom a Fast Fourier Transform on an audio buffer and return its data.
     *
     * @param audioFile A JUCE library audio sample buffer with the audio samples inside.
     * @return std::shared_ptr<std::vector<float>>  A vector of resulting fourier transform.
     */
    std::shared_ptr<std::vector<float>> performFft(std::shared_ptr<juce::AudioSampleBuffer> audioFile);

  private:
    /**
     * @brief Main loop of the threads that are performing FFT.
     *
     */
    void fftThreadsLoop();

    bool exiting;                                           /**< Do threads needs to exit ? */
    std::mutex queueMutex;                                  /**< Mutex for the job queue */
    std::condition_variable mutexCondition;                 /**< For the thread to poll on jobs or termination */
    std::vector<std::thread> workerThreads;                 /**< list of worker threads */
    std::queue<std::shared_ptr<FftRunnerJob>> todoJobQueue; /**< queue of jobs to be picked by workers */
    std::queue<std::shared_ptr<FftRunnerJob>>
        emptyJobPool;          /**< Preallocated structures to carry job information. If empty, please wait. */
    std::mutex emptyJobsMutex; /**< Prevent race condition if many threads want to run FFTs */
};

#endif // DEF_FFT_RUNNER_HPP