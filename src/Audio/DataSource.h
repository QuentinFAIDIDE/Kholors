#ifndef DATA_SOURCE_HPP
#define DATA_SOURCE_HPP

/**
Interface to have a class being
able to feed a vumeter some data.
*/
class VuMeterDataSource
{
    /**
    Return data for vu meters using their unique identifier.
    */
    virtual std::pair<float, float> getVuMeterValue(std::string vuMeterId) = 0;
};

#endif // DATA_SOURCE_HPP