#ifndef MIXBUS_DATA_SOURCE_HPP
#define MIXBUS_DATA_SOURCE_HPP

#include "DataSource.h"
#include <map>

/**
 * @brief      This class describes a mixbus data source.
 */
class MixbusDataSource : public VuMeterDataSource
{
  public:
    /**
     * @brief      Gets the vu meter value.
     *
     * @param[in]  vuMeterId  The vu meter identifier
     *
     * @return     The vu meter value.
     */
    std::pair<float, float> getVuMeterValue(VumeterId vuMeterId);

    /**
     * @brief      Called from the audio thread. Swap the maps
     * values with the previous one so that new values can be read by
     * getVuMeterValues.
     *
     * @param[in]  avalues  The map with the new values to swap
     */
    void swapVuMeterValues(std::map<VumeterId, std::pair<float, float>> values);

  private:
};

#endif // MIXBUS_DATA_SOURCE_HPP