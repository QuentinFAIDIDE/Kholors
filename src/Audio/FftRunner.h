#ifndef DEF_FFT_RUNNER_HPP
#define DEF_FFT_RUNNER_HPP

#include <condition_variable>
#include <fftw3.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <memory>
#include <mutex>
#include <vector>

#include "../Config.h"
#include "../WaitGroup.h"

// a cool post about C++ thread pools: https://stackoverflow.com/a/32593825

/**< Number of preallocated structure to pass FFT response data. 8192 is approximately 130Mb and rougthly 80 seconds of
 * stereo signal (# is linear to Mb) */
#define FFT_PREALLOCATED_JOB_STRUCTS 4096

/**< How many jobs we will sent together. Beware to keep number of job posting threads (not fft thread pool) below
 * FFT_PREALLOCATED_JOB_STRUCTS / FFT_JOBS_BATCH_SIZE */
#define FFT_JOBS_BATCH_SIZE 512

/**< Necessary correction for freq bins amplitudes for the Hanning window function.
 *  See https://community.sw.siemens.com/s/article/window-correction-factors */
#define HANN_AMPLITUDE_CORRECTION_FACTOR 2.0f

/**< jobs that are posted in the job queue and picked by threads  */
struct FftRunnerJob
{
    int position;       /**< Position of the fft in the source audio buffer (in index of ffts) */
    const float *input; /**< Audio intensities as inputs. Must be readable up to input+(sizeof(float)*inputLength) */
    float output[FFT_OUTPUT_NO_FREQS]; /**< frequency bins as output */
    float inputLength;                 /**< how many samples in the input are to be picked (from start) */
    std::shared_ptr<WaitGroup> wg;     /**< Synchronisation utility for batch of jobs across threads */
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

    /**
     * @brief Processes a job using the provided fftw processing plan.
     *
     * @param jobRef A reference to the job data object.
     * @param plan A FFTW library optimized processing plan for floats.
     * @param in The input FFTW data (to copy job input into)
     * @param out The output FFTW data (to copy job output from)
     */
    void processJob(std::shared_ptr<FftRunnerJob> jobRef, fftwf_plan *plan, float *in, fftwf_complex *out);

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
        emptyJobPool;                   /**< Preallocated structures to carry job information. If empty, please wait. */
    std::mutex emptyJobsMutex;          /**< Prevent race condition if many threads want to run FFTs */
    std::mutex fftwMutex;               /**< Mutex for non thread safe fftw init functions */
    std::vector<float> hannWindowTable; /**< factors of the hann windowing function for our desired input size */
};

#endif // DEF_FFT_RUNNER_HPP