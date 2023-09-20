#include "FftRunner.h"
#include <cstring>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <unistd.h>
#include <vector>

FftRunner::FftRunner() : exiting(false)
{

    // preallocate jobs data structures
    for (int i = 0; i < FFT_PREALLOCATED_JOB_STRUCTS; i++)
    {
        auto newEmptyJob = std::make_shared<FftRunnerJob>();
        emptyJobPool.push(newEmptyJob);
    }

    // TODO: precompute hanning windowing function based on fft windowing size

    // Pick the number of threads and start them.
    // Copy pasted from the post linked in the header file, it's already perfect like this.
    const uint32_t num_threads = std::thread::hardware_concurrency(); // Max # of threads the system supports
    for (uint32_t ii = 0; ii < num_threads; ++ii)
    {
        workerThreads.emplace_back(std::thread(&FftRunner::fftThreadsLoop, this));
    }
}

FftRunner::~FftRunner()
{
}

int FftRunner::getNumFftFromNumSamples(int numSamples)
{
    // how many fft windows we can fit if we pad the end with zeros
    int numWindowsWithPadding = std::ceil(float(numSamples) / float(FFT_INPUT_NO_INTENSITIES));
    // this formula get the exact amount of available windowed bins.
    return (numWindowsWithPadding * FFT_OVERLAP_DIVISION) - (FFT_OVERLAP_DIVISION - 1);
}

std::shared_ptr<std::vector<float>> FftRunner::performFft(std::shared_ptr<juce::AudioSampleBuffer> audioFile)
{
    // NOTE: one job = one fft

    // number of jobs to send per channel
    int noJobsPerChannel = getNumFftFromNumSamples(audioFile->getNumSamples());

    // compute size (in # of floats!) and allocate response array
    int respArraySize = audioFile->getNumChannels() * noJobsPerChannel * FFT_OUTPUT_NO_FREQS;
    auto result = std::make_shared<std::vector<float>>((size_t)respArraySize);

    // jobs sent in the current batch
    std::vector<std::shared_ptr<FftRunnerJob>> batchJobs;
    batchJobs.reserve(FFT_JOBS_BATCH_SIZE);

    // waitgroup for successive batches of jobs
    auto wg = std::make_shared<WaitGroup>();

    // repeat for each channel
    for (int ch = 0; ch < audioFile->getNumChannels(); ch++)
    {
        // channel offset in the destination array (result)
        size_t channelResultArrayOffset = (size_t)ch * (size_t)noJobsPerChannel * FFT_OUTPUT_NO_FREQS;

        // pointer to the start of the next job
        const float *nextJobStart = audioFile->getReadPointer(ch);

        // total jobs still to be sent for this channel
        int remainingJobs = noJobsPerChannel;

        // index of the fft in posted jobs
        int fftPosition = 0;

        // keep sending batch of jobs to thread while we are not finished
        while (remainingJobs != 0)
        {
            // first get as many preallocated jobs as possible
            batchJobs.clear();
            {
                int maxJobsToPick = juce::jmin(FFT_JOBS_BATCH_SIZE, remainingJobs);
                std::scoped_lock<std::mutex> lock(emptyJobsMutex);
                for (int i = 0; i < maxJobsToPick; i++)
                {
                    if (emptyJobPool.empty())
                    {
                        break;
                    }
                    batchJobs.emplace_back(emptyJobPool.front());
                    emptyJobPool.pop();
                }
            }

            // If we have not been able to pull any empty job, emit a warning and sleep
            // Note that this can happen if more than FFT_PREALLOCATED_JOB_STRUCTS / FFT_JOBS_BATCH_SIZE
            // threads are racing to use the preallocated empty jobs.
            if (batchJobs.size() == 0)
            {
                std::cerr << "Too many threads are trying to perform ffts and buffered jobs could not handle batch "
                             "size for that amount of threads!"
                          << std::endl;
                sleep(1);
                continue;
            }

            // iterate over empty job and set the appropriate input data, length, and reset bool statuses
            for (int i = 0; i < (int)batchJobs.size(); i++)
            {
                // set job properties and increment wg here
                batchJobs[(size_t)i]->input = nextJobStart;
                batchJobs[(size_t)i]->wg = wg;
                batchJobs[(size_t)i]->position = fftPosition;
                wg->Add(1);
                // decrement remaining job
                remainingJobs--;
                fftPosition++;
                // set the appropriate size
                if (remainingJobs == 0)
                {
                    batchJobs[(size_t)i]->inputLength = audioFile->getNumSamples() % FFT_INPUT_NO_INTENSITIES;
                }
                else
                {
                    batchJobs[(size_t)i]->inputLength = FFT_INPUT_NO_INTENSITIES;
                }
                // move the data pointer forward
                nextJobStart += (size_t)FFT_INPUT_NO_INTENSITIES;
            }

            // wait for waitgroup
            wg->Wait();

            // copy back the responses on we're done
            for (int i = 0; i < (int)batchJobs.size(); i++)
            {
                // helper to locate the destination area
                size_t fftChannelOffset = ((size_t)batchJobs[(size_t)i]->position * FFT_OUTPUT_NO_FREQS);
                size_t fftResultArrayOffset = channelResultArrayOffset + fftChannelOffset;
                // copy the memory from job data to destination buffer
                memcpy(result->data() + (fftResultArrayOffset * sizeof(float)), batchJobs[(size_t)i]->output,
                       sizeof(float) * FFT_OUTPUT_NO_FREQS);
            }

            // put the jobs back into the empty job queue
            for (int i = 0; i < (int)batchJobs.size(); i++)
            {
                std::scoped_lock<std::mutex> lock(emptyJobsMutex);
                emptyJobPool.push(batchJobs[(size_t)i]);
            }

            // note that the batchJobs vector is cleared on loop restart
        }
    }

    return result;
}

void FftRunner::fftThreadsLoop()
{
    // TODO: instanciate fft objects

    while (true)
    {
        std::function<void()> job;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            mutex_condition.wait(lock, [this] { return !jobs.empty() || should_terminate; });
            if (should_terminate)
            {
                return;
            }
            job = jobs.front();
            jobs.pop();
        }
        job();
    }
}

///////////////////////////////////////////
