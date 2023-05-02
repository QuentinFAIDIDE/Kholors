#ifndef DEF_ACTION_HPP
#define DEF_ACTION_HPP

#include "Marshalable.h"
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

    std::string Marshal() override;

    Marshalable *Unmarshal(std::string &) override;

    bool isCompleted();

    void setCompleted(bool c);

  private:
    bool completed;
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
    void setFailed(bool);
    bool hasFailed();
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
    bool failed;
    int newIndex;
    float splitFrequency;
};

#endif // DEF_ACTION_HPP