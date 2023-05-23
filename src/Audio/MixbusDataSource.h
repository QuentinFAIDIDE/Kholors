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
    MixbusDataSource();

    /**
     * @brief      Gets the vu meter value.
     *
     * @param[in]  vuMeterId  The vu meter identifier
     *
     * @return     The vu meter value.
     */
    juce::Optional<std::pair<float, float>> getVuMeterValue(VumeterId vuMeterId) override final;

    /**
     * @brief      Called from the audio thread. Swap the maps
     * values with the previous one so that new values can be read by
     * getVuMeterValues.
     *
     * @param[in]  avalues  The map with the new values to swap
     */
    void swapVuMeterValues(VuMeterData &values);

  private:
    juce::CriticalSection mutex;
    VuMeterData vuMeterValues;
};

#endif // MIXBUS_DATA_SOURCE_HPP