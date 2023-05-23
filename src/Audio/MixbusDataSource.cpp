#include "MixbusDataSource.h"

MixbusDataSource::MixbusDataSource()
{
}

juce::Optional<std::pair<float, float>> MixbusDataSource::getVuMeterValue(VumeterId vuMeterId)
{
    // try to lock and do nothing if unable
    const juce::CriticalSection::ScopedTryLockType lock(mutex);

    if (lock.isLocked())
    {
        return vuMeterValues[vuMeterId];
    }

    // return nothing if we can't find that vu meter data or couldn't get lock
    return juce::Optional<std::pair<float, float>>(juce::nullopt);
}

void MixbusDataSource::swapVuMeterValues(VuMeterData &newValues)
{
    // try to lock and do nothing if unable
    const juce::CriticalSection::ScopedTryLockType lock(mutex);

    if (lock.isLocked())
    {
        vuMeterValues.swap(newValues);
    }
}
