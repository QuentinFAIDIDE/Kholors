#include "SampleManager.h"
#include <iostream>

SampleManager::SampleManager() {}

SampleManager::~SampleManager() {}

bool SampleManager::filePathsValid(const juce::StringArray& files) {
    return true;
}

int SampleManager::addSample(juce::String filePath, int64_t frameIndex) {
    // for now, return -1
    return -1;
}