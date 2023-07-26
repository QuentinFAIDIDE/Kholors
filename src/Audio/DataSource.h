#ifndef DATA_SOURCE_HPP
#define DATA_SOURCE_HPP

#include <cstdint>
#include <vector>

#define VUMETER_MAP_SIZE 32

// minimum db value displayed
#define VUMETER_MIN_DB -36.0f

#include <string>
#include <utility>

#include <juce_core/juce_core.h>

/**
 * @brief      The identifiers for vu meters.
 */
enum VumeterId
{
    VUMETER_ID_NONE,     /* no vumeter set */
    VUMETER_ID_MASTER,   /* master track volume */
    VUMETER_ID_INPUT_0,  /* volume of the first input */
    VUMETER_ID_SELECTED, /* volume selected tracks */
    VUMETER_ID_LIMITER,  /* volume reduction of the limiter */
    VUMETER_SIZE         /* here for the purpose of allocating, MUST BE ALWAYS LAST */
};

/**
 * @brief      A preallocated vector that default to min db values.
 */
class VuMeterData : public std::vector<std::pair<float, float>>
{
  public:
    VuMeterData()
    {
        std::pair<float, float> min(VUMETER_MIN_DB, VUMETER_MIN_DB);
        resize(VUMETER_SIZE);
        for (size_t i = VUMETER_ID_NONE; i < VUMETER_SIZE; i++)
        {
            (*this)[i] = min;
        }
    };
};

/**
Interface to have a class being
able to feed a vumeter some data.
*/
class VuMeterDataSource
{
  public:
    /**
    Return data for vu meters using their unique identifier.
    */
    virtual juce::Optional<std::pair<float, float>> getVuMeterValue(VumeterId vuMeterId) = 0;
};

/**
 * @brief      This class describes a position data source.
 */
class PositionDataSource
{
  public:
    virtual juce::Optional<int64_t> getPosition() = 0;
};

#endif // DATA_SOURCE_HPP