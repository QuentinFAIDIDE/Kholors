#ifndef DEF_ACTION_HPP
#define DEF_ACTION_HPP

#include "../Audio/SamplePlayer.h"
#include "Marshalable.h"
#include <juce_audio_basics/juce_audio_basics.h>

class State
{
};

enum DuplicationType
{
    DUPLICATION_TYPE_NO_DUPLICATION,
    DUPLICATION_TYPE_COPY_AT_POSITION,
    DUPLICATION_TYPE_SPLIT_AT_POSITION,
    DUPLICATION_TYPE_SPLIT_AT_FREQUENCY,
};

class Task : public Marshalable
{
  public:
    Task();
    virtual ~Task();

    std::string Marshal() override;

    Marshalable *Unmarshal(std::string &) override;

    bool isCompleted();
    void setCompleted(bool c);
    void setFailed(bool);
    bool hasFailed();

  private:
    bool completed;
    bool failed;
    std::string errorMessage;
};

class SampleCreateTask : public Task
{
  public:
    // constructor for brand new samples imported from
    SampleCreateTask(std::string path, int position);
    // contructor for duplication at frequency
    SampleCreateTask(float frequency, int sampleCopyIndex);
    // contructor for other duplication tasks
    SampleCreateTask(int position, int sampleCopyIndex, DuplicationType d);

    bool isDuplication();
    int getDuplicateTargetId();
    std::string getFilePath();
    int64_t getPosition();
    void setAllocatedIndex(int);
    int getAllocatedIndex();
    float getSplitFrequency();
    DuplicationType getDuplicationType();

  private:
    DuplicationType duplicationType;
    std::string filePath;
    int editingPosition;
    bool isCopy;
    int duplicatedSampleId;
    int newIndex;
    float splitFrequency;
};

class SampleDisplayTask : public Task
{
  public:
    SampleDisplayTask(SamplePlayer *, std::shared_ptr<SampleCreateTask>);
    SamplePlayer *sample;
    std::shared_ptr<SampleCreateTask> creationTask;
};

class SampleDeletionTask : public Task
{
  public:
    SampleDeletionTask(int);
    int id;
};

class SampleDeletionDisplayTask : public Task
{
  public:
    SampleDeletionDisplayTask(int);
    int id;
};

class NotificationTask : public Task
{
  public:
    NotificationTask(std::string path);
    NotificationTask(juce::String path);
    std::string getMessage();

  private:
    std::string message;
};

class ImportFileCountTask : public Task
{
  public:
    ImportFileCountTask(std::string);
    std::string path;
};

class SampleTimeCropTask : public Task
{
  public:
    /**
     * SampleTimeCropTask constructor to record a task
     * that either crop the beginning or the end of a sample.
     * It record the sample index and the distance in frames
     * it was cropped for.
     */
    SampleTimeCropTask(bool cropBeginning, int sampleId, int frameDist);
    int id;
    int dragDistance;
    int movedBeginning;
};

class SampleFreqCropTask : public Task
{
  public:
    /**
     * Constructor to record a task
     * that either change the lowPass or highPass
     * filter frequency of a sample.
     */
    SampleFreqCropTask(bool isLP, int sampleId, float initialFreq, float finalFreq);
    int id;
    int initialFrequency;
    int finalFrequency;
    int isLowPass;
};

#endif // DEF_ACTION_HPP