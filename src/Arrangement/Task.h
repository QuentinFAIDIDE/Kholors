#ifndef DEF_ACTION_HPP
#define DEF_ACTION_HPP

#include "Marshalable.h"
class State
{
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
    SampleCreateTask(std::string path, int position);
    SampleCreateTask(int position, int sampleCopyIndex);

    bool isDuplication();
    int getDuplicateTargetId();
    std::string getFilePath();
    int64_t getPosition();
    void setFailed(bool);
    bool hasFailed();
    void setAllocatedIndex(int);
    int getAllocatedIndex();

  private:
    std::string filePath;
    int editingPosition;
    bool isCopy;
    int duplicatedSampleId;
    bool failed;
    int newIndex;
};

#endif // DEF_ACTION_HPP