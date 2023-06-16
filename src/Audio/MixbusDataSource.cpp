#include "MixbusDataSource.h"

MixbusDataSource::MixbusDataSource()
{
}

juce::Optional<std::pair<float, float>> MixbusDataSource::getVuMeterValue(VumeterId vuMeterId)
{
    // try to lock and do nothing if unable
    const juce::CriticalSection::ScopedTryLockType lock(vuMetersMutex);

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
    const juce::CriticalSection::ScopedTryLockType lock(vuMetersMutex);

    if (lock.isLocked())
    {
        vuMeterValues.swap(newValues);
    }
}

void MixbusDataSource::updateSelectedTracks(std::set<size_t> &newSelectedTracks)
{
    juce::CriticalSection::ScopedLockType lock(selectedTracksMutex);
    newSelectedTracks.swap(selectedTracks);
}

std::set<size_t> *MixbusDataSource::getLockedSelectedTracks()
{
    std::set<size_t> *response = nullptr;

    bool isLocked = selectedTracksMutex.tryEnter();

    if (isLocked)
    {
        response = &selectedTracks;
    }

    return response;
}

void MixbusDataSource::releaseSelectedTracks()
{
    selectedTracksMutex.exit();
}
