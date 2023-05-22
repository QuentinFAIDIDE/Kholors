#ifndef DATA_SOURCE_HPP
#define DATA_SOURCE_HPP

#include <string>
#include <utility>

/**
 * @brief      The identifiers for vu meters.
 */
enum VumeterId
{
    VUMETER_ID_MASTER,   /* master track volume */
    VUMETER_ID_INPUT_0,  /* volume of the first input */
    VUMETER_ID_SELECTED, /* volume selected tracks */
    VUMETER_ID_LIMITER   /* volume reduction of the limiter */
};

/**
Interface to have a class being
able to feed a vumeter some data.
*/
class VuMeterDataSource
{
    /**
    Return data for vu meters using their unique identifier.
    */
    virtual std::pair<float, float> getVuMeterValue(VumeterId vuMeterId) = 0;
};

#endif // DATA_SOURCE_HPP