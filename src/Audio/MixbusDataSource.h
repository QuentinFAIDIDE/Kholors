#ifndef MIXBUS_DATA_SOURCE_HPP
#define MIXBUS_DATA_SOURCE_HPP

#include "DataSource.h"
#include <map>

/**
 * @brief      This class describes a mixbus data source.
 *             Its holds copy of informations like volumes
 *             for vu meters or currently selected tracks.
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

    /**
     * @brief      Receives a copy of the set holding the selected
     *             tracks ids and takes care of swapping with the locked
     *             ones that are requested by the mixbus. This function can
     *             lock.
     *
     * @param[in]  newSelectedTracks  The new selected tracks
     */
    void updateSelectedTracks(std::set<size_t> &newSelectedTracks);

    /**
     * @brief      Gets the selected tracks. Will not block and
     *             return nothing if current selection is locked.
     *             Warning: it takes the lock untill releaseSelectedTracks()
     *             is called.
     *
     * @return     The selected tracks.
     */
    std::set<size_t> *getLockedSelectedTracks();

    /**
     * @brief      Releases the lock on selected tracks. To call after
     *             getLockedSelectedTracks. Warning: quite dangerous
     *             to use ! Never ever allow an udpateSelectedTracks to be
     *             called and return something != nullptr without a matching call
     *             to this to release lock.
     */
    void releaseSelectedTracks();

  private:
    juce::CriticalSection vuMetersMutex;
    VuMeterData vuMeterValues;

    std::set<size_t> selectedTracks;
    juce::CriticalSection selectedTracksMutex;
};

#endif // MIXBUS_DATA_SOURCE_HPP