#include "MixbusDataSource.h"

std::pair<float, float> MixbusDataSource::getVuMeterValue(VumeterId vuMeterId)
{
    const juce::ScopedLock sl(mutex);

    auto research = vuMeterValues.find(vuMeterId);
    if(research != vuMeterValues.end())
    {
        return *research;
    }
    // return nothing if we can't find that vu meter data
    else
    {
        return std::pair<float, float>(0,0);
    }
}

void MixbusDataSource::swapVuMeterValues(std::map<VumeterId, std::pair<float, float>> &newValues)
{
    // try to lock and do nothing if unable
    const juce::SpinLock::ScopedTryLockType lock(mutex);

    if (lock.isLocked())
    {
        vuMeterValues.swap(newValues);
    }
}
