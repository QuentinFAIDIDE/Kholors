#ifndef DEF_ACTION_HPP
#define DEF_ACTION_HPP

#include "../Audio/SamplePlayer.h"
#include "Marshalable.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <memory>

#define MAX_TASK_INDEX 1048576

#define RESET_TASK_NO_STEPS 3

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
    std::string marshal() override;

    /**
      Parse the task from a string.
     */
    void unmarshal(std::string &) override;

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
      The tasks are performed in order, ie lowest ids first.
     */
    virtual std::vector<std::shared_ptr<Task>> getOppositeTasks();

    /**
     Declare this task as part of a reversion (eg obtained with getOppositeTasks).
     */
    void declareSelfAsPartOfReversion();

    /**
     Declare this task as not going into the task history (used for restoring canceled actions mostly)
     */
    void preventFromGoingToTaskHistory();

    /**
     Called when the task is about to be posted a second time after a rewind. Example of specific use case:
     SampleCreateTask to reuse ids.
     */
    virtual void prepareForRepost();

    /**
     * @brief      Gets the task group index.
     *
     * @return     The task group index.
     */
    int getTaskGroupIndex();

    /**
     * @brief      Sets the task group index.
     *
     * @param[in]  newTaskGroupIndex  The new task group index
     */
    void setTaskGroupIndex(int newTaskGroupIndex);

    /**
     * @brief      Sets the task group to same of the target.
     *
     * @param      targeTask  The target task
     */
    void setTaskGroupToTarget(Task &targetTask);

    /**
     * @brief      Gets a new task group index (incremented from task index counter).
     *
     * @return     The new task group index.
     */
    static int getNewTaskGroupIndex();

  protected:
    bool recordableInHistory; // tell if this task should be saved in history. Inherit SilentTask to have it false.
    bool isPartOfReversion;   // tells if this task was obtained through getOppositeTasks

  private:
    bool completed;           // has this task been completed/performed
    bool failed;              // has this task failed
    std::string errorMessage; // an eventual error messaged for the task failure

    int taskGroupIndex; // a unique identifier to group tasks together

    static int taskGroupIndexIterator; // a static value that is incremented and assigned to new task, and also wrapped
                                       // at MAX_TASK_INDEX
};

/**
 * @brief      This class describes a silent task. This is a task
 *             that defaults to not being recordable in history.
 */
class SilentTask : public Task
{
  public:
    SilentTask();
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
    std::vector<std::shared_ptr<Task>> getOppositeTasks() override;

    /**
     Set the low pass and high pass filter frequencies. This is
     import for the getOppositeTasks to be able to restore them
     with new filtering tasks.
     */
    void setFilterInitialFrequencies(float highPass, float lowPass);

    /**
     Save the duplicated sample original length in audio frames.
     This is used to reverse the duplication task (where the length is changed)
     */
    void setSampleInitialLength(int);

    /**
     Tells the tasks executor (MixingBus class) to reuse a previous allocated
     id and to reset task status to not completed and not failed.
     */
    void prepareForRepost() override;

    /**
     Dumps the task data to a string as json
     */
    std::string marshal() override;

    DuplicationType duplicationType; // is this a new sample, duplication at position, split at freq, or split at pos
    std::string filePath;            // the path to the file if it's a new sample
    int position;                    // the position of import or split
    bool isCopy;                     // is this a duplication ?
    int duplicatedSampleId;          // the identifier of the duplicated sample
    int newIndex;                    // the index of the newly created sample
    float splitFrequency;            // the frequency where it's split
    float lowPassFreq;               // frequency of the original sample low pass filter
    float highPassFreq;              // frequency of the original high pass filter
    int originalLength;              // the length of the sample before it was duplicated / splitted
    bool reuseNewId;                 // this is set when user undo and redo and we reuse index to preserve other tasks
};

/**
  A task to display a newly created sample. Serves the purpose
  of passing the info that a new sample was indeed created from the
  mixbus to the ArrangementArea.
 */
class SampleDisplayTask : public SilentTask
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

    /**
     Dumps the task data to a string as json
     */
    std::string marshal() override;

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

    /**
      Get the list of tasks that will revert the current task.
      This one just spawns a SampleRestoreTask.
     */
    std::vector<std::shared_ptr<Task>> getOppositeTasks() override;

    /**
     Dumps the task data to a string as json
     */
    std::string marshal() override;

    int id;                                      // the sample id to delete
    std::shared_ptr<SamplePlayer> deletedSample; // the sample we just deleted
};

/**
  Task to restore a sample after it was previously deleted.
 */
class SampleRestoreTask : public Task
{
  public:
    /**
     Create a task that will reset the sample at given id after it has been deleted.
     */
    SampleRestoreTask(int index, std::shared_ptr<SamplePlayer> sampleToRestore);

    /**
     Dumps the task data to a string as json
     */
    std::string marshal() override;

    int id;                                        // id of the previously deleted sample where we need to restore
    std::shared_ptr<SamplePlayer> sampleToRestore; // reference to the deleted sample to restore
};

/**
  Task to display a deleted sample.
 */
class SampleDeletionDisplayTask : public SilentTask
{
  public:
    /**
    Creates a task to display the sample with this id.
     */
    SampleDeletionDisplayTask(int);

    int id; // the id of the sample to hide
};

/**
 Task to display this sample that was previously deleted.
 */
class SampleRestoreDisplayTask : public SilentTask
{
  public:
    /**
     Display the sample at the id again, using the reference
     to its SamplePlayer.
     */
    SampleRestoreDisplayTask(int, std::shared_ptr<SamplePlayer>);

    int id;                                       // id of the sample to restore
    std::shared_ptr<SamplePlayer> restoredSample; // object that holds the sample data
};

/**
 Task to pop a notification.
 */
class NotificationTask : public SilentTask
{
  public:
    /**
     Task to pop a notif from a std string.
     */
    NotificationTask(std::string path);
    /**
     Task to pop a notif from a juce string
     */
    NotificationTask(juce::String path);

    /**
     Return the string message that belongs in the notif.
     * @return text of the notification
     */
    std::string getMessage();

    /**
     Dumps the task data to a string as json
     */
    std::string marshal() override;

  private:
    std::string message; // notification text message
};

/**
 Task to increment the count of usage of this
 audio file.
 */
class ImportFileCountTask : public SilentTask
{
  public:
    /**
     Create the task to increment the usage count for
     this file path.
     */
    ImportFileCountTask(std::string fileFullPath);

    std::string path; // path to the file to increment import count for
};

/**
 Task to move an edge of the audio sample on the arrangement area.
 If it's the left edge, it will shift the beginning in audio file and the position
 of the sample. If it's the right edge, it will just change its length its read
 up to.
 It will also be responsible for tracking fade in and fade out gain ramps changes
 for the resized sample (resize can alter these values). You should AVOID AT ALL COST
 to treat this task (emit it completed and undoable (reversable) and going in history)
 if you have not set all four fade frame lengths !!!
 */
class SampleTimeCropTask : public Task
{
  public:
    /**
      SampleTimeCropTask constructor to record a task
      that either crop the beginning or the end of a sample.
      It record the sample index and the distance in frames
      it was cropped for.
     */
    SampleTimeCropTask(bool cropBeginning, int sampleId, int frameDist);

    /**
     Dumps the task data to a string as json
     */
    std::string marshal() override;

    /**
      Get the list of tasks that will revert the current task.
      This one just spawns a SampleTimeCropTask with opposite
      dragDisance.
     */
    std::vector<std::shared_ptr<Task>> getOppositeTasks() override;

    int id;               // id of sample to crop
    int dragDistance;     // the distance in audio frame to move from
    bool movingBeginning; // are we moving the left edge (beginning) or the right one (end)

    // we record the fade ins and out before and after
    // because we might alter them and therefore
    // need to restore them on cancellation task
    int initialFadeInFrameLen;  // length in frame of the fade in before resizing
    int initialFadeOutFrameLen; // length in frame of the fade out before resizing
    int finalFadeInFrameLen;    // length in frame of the fade in after resizing
    int finalFadeOutFrameLen;   // length in frame of the fade out after resizing
};

/**
 Task to move the high pass or low pass filter of a sample.
 */
class SampleFreqCropTask : public Task
{
  public:
    /**
     * Constructor to record a task
     * that either change the lowPass or highPass
     * filter frequency of a sample.
     */
    SampleFreqCropTask(bool isLP, int sampleId, float initialFreq, float finalFreq);

    /**
     Dumps the task data to a string as json
     */
    std::string marshal() override;

    /**
      Get the list of tasks that will revert the current task.
      This one just spawns a SampleFreqCropTask with swapped initial
      and final freq.
     */
    std::vector<std::shared_ptr<Task>> getOppositeTasks() override;

    int id;                 // id of sample to edit
    float initialFrequency; // the frenquency before crop
    float finalFrequency;   // the frequency after crop
    bool isLowPass;         // is this the low pass or the high pass filter ?
};

/**
 Task to move a sample.
 */
class SampleMovingTask : public Task
{
  public:
    /**
      Record when a sample is moved on the
      arrangement area.
     */
    SampleMovingTask(int sampleId, int frameDist);

    /**
     Dumps the task data to a string as json
     */
    std::string marshal() override;

    /**
      Get the list of tasks that will revert the current task.
      This one just spawns a SampleMovingTask with opposite dragDistance.
     */
    std::vector<std::shared_ptr<Task>> getOppositeTasks() override;

    int id;           // sample to be moved
    int dragDistance; // distanced it's moved by (can be negative)
};

/**
 Task to update displayed information of sample.
 */
class SampleUpdateTask : public SilentTask
{
  public:
    /**
      Task to update the displayed sample
      on screen. ArrangementArea will
      pass the sampleplayer object to
      the opengl handler that will reload
      the model.
     */
    SampleUpdateTask(int sampleId, std::shared_ptr<SamplePlayer> sp);

    /**
     Dumps the task data to a string as json
     */
    std::string marshal() override;

    int id;                               // sample to be updated
    std::shared_ptr<SamplePlayer> sample; // reference to SamplePlayer object
};

/**
 Task to recolor selected sample groups.
 */
class SampleGroupRecolor : public Task
{
  public:
    /**
      Task to recolor selected samples
     */
    SampleGroupRecolor(int colorId);

    /**
      Task to recolor selected samples, but using
      raw color and set of track ids instead (for reversed version)
     */
    SampleGroupRecolor(std::set<int>, std::map<int, juce::Colour>);

    /**
     Dumps the task data to a string as json
     */
    std::string marshal() override;

    /**
      Get the opposite colour change
     */
    std::vector<std::shared_ptr<Task>> getOppositeTasks() override;

    // id of the color to put (from the colorPalette)
    int colorId;
    // the ids of the samples that were changed
    std::set<int> changedSampleIds;
    // the color before the change
    juce::Colour colorPerIds;
    // the color after the change
    juce::Colour newColor;
    // list of colors per id
    std::map<int, juce::Colour> colorsPerId;
    // are we recoloring from raw value or id
    bool colorFromId;
};

/**
That that informs about changes to selection in ArrangementArea
to replicate them in the Mixbus selection buffer.
*/
class SelectionChangingTask : public SilentTask
{
  public:
    /**
    Called to create the  task using the newSelection.
    */
    SelectionChangingTask(std::set<size_t> &newSelection);

    /**
    Dumps the task data to a string as json
    */
    std::string marshal() override;

    std::set<size_t> newSelectedTracks;
};

/**
 * @brief      This class describes a numeric input update task.
 *             This will update the backend value based on
 *             the new input value.
 */
class NumericInputUpdateTask : public Task
{

  public:
    /**
     * @brief      Constructs a new instance. This one
     *             will be silent and will not be recorded in
     *             history. Used during dragging of numeric input values.
     *
     * @param[in]  inputId   The input identifier
     * @param[in]  val       The new value to set
     */
    NumericInputUpdateTask(int inputId, float val);

    /**
     * @brief      Constructs a new instance. This one
     *             will be marked completed
     *             and will be recorded in history to be
     *             available for reversion.
     *
     * @param[in]  inputId   The input identifier
     * @param[in]  val       The value
     * @param[in]  oldValue  The old value
     */
    NumericInputUpdateTask(int inputId, float val, float oldValue);

    /**
     * @brief      This task simply asks to be assigned the current
     *             value of the input and marked completed.
     *             This is generally emitted by the NumericInput component
     *             so that it can get the value through the completed event.
     *
     * @param[in]  inputId   The input identifier
     */
    NumericInputUpdateTask(int inputId);

    /**
    Dumps the task data to a string as json
    */
    std::string marshal() override;

    /**
      Get the opposite task with flipped old and new value. Returns
      nothing if the rask in not reversible (ie build with the constructor)
      that does not expect oldValue
     */
    std::vector<std::shared_ptr<Task>> getOppositeTasks() override;

    // the identifier of the input
    int numericalInputId;

    // the new value to be set (or that has been set)
    float newValue;

    // the old value that was there at before. Will be
    // set only if the task was created through second
    // constructor that allows to set this. This means
    // that it is recorded in history and can be reverted
    // unlike task created with the other constructor.
    float oldValue;

    // this is true when the third lighter constructor was
    // used (where only id is provided). It means that
    // the tasks must be assigned the current value
    // of the numeric input in newValue and oldValue
    // and be marked completed and broadcasted again.
    bool isBroadcastRequest;
};

/**
 * @brief      Task to start or stop playing the current track.
 *             Optionally it can be set to broadcast its current
 *             state and reset position to zero.
 */
class PlayStateUpdateTask : public SilentTask
{
  public:
    /**
     * @brief      Constructs a new instance that will
     *             request the mixbus to populate the
     *             isCurrentlyPlaying member to true of its playing,
     *             false otherwise.
     */
    PlayStateUpdateTask();

    /**
     * @brief      Constructs a new instance that will
     *             start playing if play is true, or pause
     *             otherwise. If resetPositionOnPause is true
     *             it will reset the position to zero if pausing.
     *
     * @param[in]  play  The play
     */
    PlayStateUpdateTask(bool play, bool resetPosition);

    /**
    Dumps the task data to a string as json
    */
    std::string marshal() override;

    bool shouldPlay, shouldResetPosition, isCurrentlyPlaying, requestingStateBroadcast;
};

/**
 * @brief    Task to toggle on or off the loop mode.
 */
class LoopToggleTask : public SilentTask
{
  public:
    /**
     * @brief   Constructs a new instance that will ask
     *          for the current looping state to be saved
     *          into the isCurrentlyLooping variable.
     *          It will not change the looping state.
     */
    LoopToggleTask();

    /**
     * @brief   Constructs a new instance that will ask
     *          for the current looping state to be toggled
     *          on or off depending on the bool argument.
     */
    LoopToggleTask(bool on);

    /**
    Dumps the task data to a string as json
    */
    std::string marshal() override;

    bool shouldLoop, isCurrentlyLooping, requestingStateBroadcast;
};

class LoopMovingTask : public Task
{
  public:
    /**
     * @brief    Constructs a new tasks that will
     *           broadcast current loop position
     *           as a completed task with currnetLoopBeginFrame
     *           and actualLoopEndFrame populated with
     *           the values
     */
    LoopMovingTask();

    /**
     * @brief    Constructs a new tasks that will
     *           set current loop position to desired
     *           one passed as args. Will not record
     *           the task, the recorded one is the next
     *           constructor that is reversable.
     *           This is used when user start dragging and
     *           we want to reflect the change, but don't want
     *           to record a thousands micro movement for CTRL+Z.
     *           When done dragging, the component will emit
     *           a completed LoopMovingTask that will be reversable.
     */
    LoopMovingTask(int64_t newBegin, int64_t newEnd);

    /**
     * @brief    Constructs a new task that will
     *           be marked completed and will be saved in history.
     *           Used to complete a drag movement and make it reversible.
     */
    LoopMovingTask(int64_t oldBegin, int64_t oldEnd, int64_t newBegin, int64_t newEnd);

    /**
     * @brief      Quantisize the current currentLoopBeginFrame and currentLoopEndFrame values
     *             effectively bringing them to the nearest multiple of quantLevel.
     *
     * @param[in]  quantLevel  The quant level
     */
    void quantisize(float quantLevel);

    /**
    Dumps the task data to a string as json
    */
    std::string marshal() override;

    /**
      Get the opposite task with flipped old and new value
     */
    std::vector<std::shared_ptr<Task>> getOppositeTasks() override;

    bool isBroadcastRequest;
    int64_t previousLoopBeginFrame, previousLoopEndFrame, currentLoopBeginFrame, currentLoopEndFrame;
};

class SampleFadeChange : public Task
{

  public:
    SampleFadeChange(int id);
    SampleFadeChange(int id, int newFadeInFrameLen, int newFadeOutFrameLen);
    SampleFadeChange(int id, int oldFadeInFrameLen, int oldFadeOutFrameLen, int newFadeInFrameLen,
                     int newFadeOutFrameLen);

    /**
    Dumps the task data to a string as json
    */
    std::string marshal() override;

    /**
      Get the opposite task with flipped old and new value
     */
    std::vector<std::shared_ptr<Task>> getOppositeTasks() override;

    int sampleId;
    bool onlyFadeIn, onlyFadeOut;
    int previousFadeInFrameLen, previousFadeOutFrameLen, currentFadeInFrameLen, currentFadeOutFrameLen;
    bool isBroadcastRequest;
};

class SampleGainChange : public Task
{

  public:
    /**
     * @brief      Constructs a new instance. This one will not induce
     *             value change but rather ask for the actual value to be
     *             set in this task object so that the creator of the task
     *             can read back the value in the completed object. It will
     *             not be recorded in history.
     *
     * @param[in]  id    The identifier
     */
    SampleGainChange(int id);

    /**
     * @brief      Constructs a new instance. It will try to set this value
     *             and will not be recorded in history.
     *
     * @param[in]  id      The identifier
     * @param[in]  dbGain  The database gain
     */
    SampleGainChange(int id, float dbGain);

    /**
     * @brief      This tasks is set completed when created and is recorded in history.
     *
     * @param[in]  id              The identifier
     * @param[in]  previousGainDb  The previous gain database
     * @param[in]  currentGainDb   The current gain database
     */
    SampleGainChange(int id, float previousGainDb, float currentGainDb);

    /**
    Dumps the task data to a string as json
    */
    std::string marshal() override;

    /**
      Get the opposite task with flipped old and new value
     */
    std::vector<std::shared_ptr<Task>> getOppositeTasks() override;

    int sampleId;
    float previousDbGain, currentDbGain;
    bool isBroadcastRequest;
};

class SampleFilterRepeatChange : public Task
{
  public:
    /**
     * @brief      Constructs a new instance. It will try to set this value
     *             and will not be recorded in history.
     *
     * @param[in]  id    The identifier of the sample
     * @param[in]  isLP  Indicates if it's for low pass (and if not, it's a high pass)
     */
    SampleFilterRepeatChange(int id, bool isLP);

    /**
     * @brief      Constructs a new instance. It will try to set this value
     *             and will not be recorded in history.
     *
     * @param[in]  id      The identifier of the sample
     * @param[in]  isLP    Indicates if it's for low pass (and if not, it's a high pass)
     * @param[in]  repeat  The number of times the filter repeats
     */
    SampleFilterRepeatChange(int id, bool isLP, int repeat);

    /**
     * @brief      This tasks is set completed when created and is recorded in history.
     *
     * @param[in]  id              The identifier of the sample
     * @param[in]  isLP  Indicates if it's for low pass (and if not, it's a high pass)
     * @param[in]  previousRepeat  The previous repeat
     * @param[in]  newRepeat       The new repeat
     */
    SampleFilterRepeatChange(int id, bool isLP, int previousRepeat, int newRepeat);

    /**
    Dumps the task data to a string as json
    */
    std::string marshal() override;

    /**
      Get the opposite task with flipped old and new value
     */
    std::vector<std::shared_ptr<Task>> getOppositeTasks() override;

    // the identifier of the sample that we modify
    int sampleId;

    // the old and new filter repeat value (how many time the filter is repeated)
    float previousFilterRepeat, newFilterRepeat;

    // is this a low pass filter or a high pass ?
    bool isLowPassFilter;

    // is this task a request to broacast the actual value
    bool isBroadcastRequest;
};

/**
 * @brief      This class describes a quitting task. It will
 *             exit the software and close the window.
 */
class QuittingTask : public SilentTask
{
  public:
    /**
    Dumps the task data to a string as json
    */
    std::string marshal() override;
};

/**
 * @brief      This class describes a reset task that is in charge
 *             of resetting the whole software state as if it was
 *             just started.
 */
class ResetTask : public Task
{
  public:
    ResetTask();

    void markStepDoneAndCheckCompletion();

    /**
    Dumps the task data to a string as json
    */
    std::string marshal() override;

    int noStepsRemaining;
};

enum OpenProjectTaskStage
{
    OPEN_PROJECT_STAGE_APP_STATE_SETUP = 0,
    OPEN_PROJECT_STAGE_TAXONOMY_SETUP = 1,
    OPEN_PROJECT_STAGE_ARRANGEMENT_SETUP = 2,
    OPEN_PROJECT_STAGE_MIXBUS_SETUP = 3,
    OPEN_PROJECT_STAGE_COMPLETED = 4,
    OPEN_PROJECT_STAGE_FAILED = 5
};

/**
 * @brief      This class describes a project opening task.
 *             It will pass through different stages through
 *             activity manager that will allow it to trigger
 *             state loading in the various app components.
 */
class OpenProjectTask : public Task
{
  public:
    /**
     * @brief      Constructs a new instance and parse files.
     *
     * @param[in]  projectPath  The project path
     */
    OpenProjectTask(std::string projectPath);

    /**
    Dumps the task data to a string as json
    */
    std::string marshal() override;

    std::string projectFolderPath;
    OpenProjectTaskStage stage;

    std::string appStateConfig;
    std::string taxonomyConfig;
    std::string uiConfig;
    std::string mixbusConfig;
};

/**
 * @brief      A task responsible for triggering
 *             track saving and git repository initialization.
 */
class GitRepoInitTask : public Task
{
  public:
    GitRepoInitTask(std::string name);

    /**
    Dumps the task data to a string as json
    */
    std::string marshal() override;

    std::string name;
};

/**
 * @brief      A task responsible for triggering
 *             track saving and commiting.
 */
class GitCommitTask : public Task
{
  public:
    GitCommitTask(std::string s);

    /**
    Dumps the task data to a string as json
    */
    std::string marshal() override;

    std::string message;
};

#endif // DEF_ACTION_HPP