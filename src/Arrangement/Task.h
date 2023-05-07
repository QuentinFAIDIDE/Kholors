#ifndef DEF_ACTION_HPP
#define DEF_ACTION_HPP

#include "../Audio/SamplePlayer.h"
#include "Marshalable.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <memory>

/**
  Unused as of now.
 */
class State
{
};

/**
  When a sample is created, it's one
  of those duplication types.
 */
enum DuplicationType
{
    DUPLICATION_TYPE_NO_DUPLICATION,    // plain new sample
    DUPLICATION_TYPE_COPY_AT_POSITION,  // copied from another and moved to a certain position
    DUPLICATION_TYPE_SPLIT_AT_POSITION, // take an existing sample, reduce it to be before some position, and create the
                                        // cropped out part after as a new sample
    DUPLICATION_TYPE_SPLIT_AT_FREQUENCY, // take an existing sample, filter it to be below some frequency, and create a
                                         // new sample with the filtered out part
};

/**
  Abstract class for "Tasks", which are actions to perform or that were
  already performed. These Tasks are sent down the ActivityManager
  broadcasting functions to TaskListeners.

  The goals are to isolate the classes by having a shared task channel (less deps injection),
  allowing for easy task broadcast by any new class (eg. imagine a
  new rpc api working on top of the UI), and to allow recording and reverting tasks
  in history.
 */
class Task : public Marshalable
{
  public:
    /**
      Constructor
     */
    Task();

    /**
      Virtual destructor
     */
    virtual ~Task();

    /**
      Convert the task into a string.
     */
    std::string Marshal() override;

    /**
      Parse the task from a string.
     */
    Marshalable *Unmarshal(std::string &) override;

    /**
      Has the task been completed already ?
     */
    bool isCompleted();

    /**
      Set the completion state of the task.
      @param c true if task was completed, false otherwise
     */
    void setCompleted(bool c);

    /**
      Set the failed state of the task.
      @param bool true if the task has failed, false otherwise.
     */
    void setFailed(bool);

    /**
      Has that task failed ?
     */
    bool hasFailed();

    /**
      Should this task be recorded in history ?
     */
    bool goesInTaskHistory();

    /**
      Get the sequence of opposite task from this one, eg the ones
      that when performed cancels out this task.
      If this is not possible, the array is empty.
     */
    virtual std::vector<std::shared_ptr<Task>> getReversed();

  protected:
    bool recordableInHistory; // tell if this task should be saved in history
    bool reversed;            // tells if this task is the reversed version of some other

  private:
    bool completed;           // has this task been completed/performed
    bool failed;              // has this task failed
    std::string errorMessage; // an eventual error messaged for the task failure
};

class SampleCreateTask : public Task
{
  public:
    /**
        constructor for brand new samples import
     */
    SampleCreateTask(std::string path, int position);

    /**
        contructor for duplication at frequency
     */
    SampleCreateTask(float frequency, int sampleCopyIndex);

    /**
      contructor for duplication tasks that are not at frequency and based on a position
     */
    SampleCreateTask(int position, int sampleCopyIndex, DuplicationType d);

    /**
      is this a new sample duplicated from another
     */
    bool isDuplication();

    /**
      Get the identifier of the sample we are duplicating.
     */
    int getDuplicateTargetId();

    /**
       when the sample is not a duplication, returns its path
       to the raw audio file.
     */
    std::string getFilePath();

    /**
       Get the position the sample is inserted at if it's not a
       duplication, or get its split position if it's a time based
       split.
     */
    int64_t getPosition();

    /**
      This sets the sample id that was assigned to the newly created
      sample.
     */
    void setAllocatedIndex(int);

    /**
      This gets the sample id that was assigned to the newly created
      sample.
     */
    int getAllocatedIndex();

    /**
      This returns the frequency we're splitting the sample at.
      (Use if the split is frequency based)
     */
    float getSplitFrequency();

    /**
      Return the duplication type of the newly created sample.
      @see DuplicationType
     */
    DuplicationType getDuplicationType();

    /**
      Get the list of tasks that will revert the current task.
     */
    std::vector<std::shared_ptr<Task>> getReversed() override;

    /**
     Set the low pass and high pass filter frequencies. This is
     import for the getReversed to be able to restore them
     with new filtering tasks.
     */
    void setFilterInitialFrequencies(float highPass, float lowPass);

    /**
     Save the duplicated sample original length in audio frames.
     This is used to reverse the duplication task (where the length is changed)
     */
    void setSampleInitialLength(int);

  private:
    DuplicationType duplicationType; // is this a new sample, duplication at position, split at freq, or split at pos
    std::string filePath;            // the path to the file if it's a new sample
    int editingPosition;             // the position of import or split
    bool isCopy;                     // is this a duplication ?
    int duplicatedSampleId;          // the identifier of the duplicated sample
    int newIndex;                    // the index of the newly created sample
    float splitFrequency;            // the frequency where it's split
    float lowPassFreq;               // frequency of the original sample low pass filter
    float highPassFreq;              // frequency of the original high pass filter
    int originalLength;              // the length of the sample before it was duplicated / splitted
};

/**
  A task to display a newly created sample. Serves the purpose
  of passing the info that a new sample was indeed created from the
  mixbus to the ArrangementArea.
 */
class SampleDisplayTask : public Task
{
  public:
    /**
       Create a SampleDisplayTask with a ref to the newly created
       SamplePlayer and a shared pointer to the task which created it.
       The task shared ptr allow for the arrangement area to edit the
       taxonomy based on duplication type, and take apropriate updating
       decisions.
     */
    SampleDisplayTask(std::shared_ptr<SamplePlayer>, std::shared_ptr<SampleCreateTask>);

    std::shared_ptr<SamplePlayer> sample;           // ref to the newly created sample
    std::shared_ptr<SampleCreateTask> creationTask; // the task that created it
};

/**
  Task emmited when we want to delete a sample.
 */
class SampleDeletionTask : public Task
{
  public:
    /**
      Create a sample deletion task by passing the sample id.
     */
    SampleDeletionTask(int);
    int id; // the sample id to delete
    std::shared_ptr<SamplePlayer> deletedSample;
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

class SampleMovingTask : public Task
{
  public:
    /**
     * Record when a sample is moved on the
     * arrangement area.
     */
    SampleMovingTask(int sampleId, int frameDist);
    int id;
    int dragDistance;
};

#endif // DEF_ACTION_HPP