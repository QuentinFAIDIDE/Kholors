#ifndef DEF_SAMPLE_STATE_HPP
#define DEF_SAMPLE_STATE_HPP

#include <vector>

#include "Marshalable.h"
#include "Task.h"

class SampleState : public Marshalable, public State
{
  public:
    int id;
    std::string samplePath;
    int64_t editingPosition;
    int64_t bufferPosition;
    int64_t length;
    float lowPassFreq;
    float highPassFreq;
};

#endif // DEF_SAMPLE_STATE_HPP