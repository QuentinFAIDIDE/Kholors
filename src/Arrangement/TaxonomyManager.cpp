#include "TaxonomyManager.h"

#include <string>

#include "ColorPalette.h"

#define DEFAULT_TAXONOMY_VECTOR_SIZE 8192
#define DEFAULT_TAXONOMY_VECTOR_ROOM 1024

TaxonomyManager::TaxonomyManager()
{
    samples.reserve(DEFAULT_TAXONOMY_VECTOR_SIZE);
    groups.reserve(DEFAULT_TAXONOMY_VECTOR_SIZE);

    for (int i = 0; i < DEFAULT_TAXONOMY_VECTOR_SIZE; i++)
    {
        SampleMetadata sm;
        sm.name = "Sample " + std::to_string(i);
        sm.groupId = i;
        sm.sampleId = i;
        samples.push_back(sm);

        SampleGroup sg;
        sg.groupId = i;
        sg.name = "Group " + std::to_string(i);
        sg.sampleIds.insert(i);
        sg.color = colourPalette[i % colourPalette.size()];
        groups.push_back(sg);
    }
}

void TaxonomyManager::reset()
{
    samples.clear();
    groups.clear();

    // as we speak, these should have no effect because
    // clear does not change capacity
    samples.reserve(DEFAULT_TAXONOMY_VECTOR_SIZE);
    groups.reserve(DEFAULT_TAXONOMY_VECTOR_SIZE);

    for (int i = 0; i < DEFAULT_TAXONOMY_VECTOR_SIZE; i++)
    {
        SampleMetadata sm;
        sm.name = "Sample " + std::to_string(i);
        sm.groupId = i;
        sm.sampleId = i;
        samples.push_back(sm);

        SampleGroup sg;
        sg.groupId = i;
        sg.name = "Group " + std::to_string(i);
        sg.sampleIds.insert(i);
        sg.color = colourPalette[i % colourPalette.size()];
        groups.push_back(sg);
    }
}

void TaxonomyManager::extendElementsArrays(int index)
{
    samples.reserve(index + DEFAULT_TAXONOMY_VECTOR_ROOM);
    groups.reserve(index + DEFAULT_TAXONOMY_VECTOR_ROOM);
    while (samples.size() < (samples.capacity()))
    {
        SampleMetadata sm;
        sm.name = "Sample " + std::to_string(samples.size());
        sm.groupId = samples.size();
        sm.sampleId = samples.size();
        samples.push_back(sm);

        SampleGroup sg;
        sg.groupId = groups.size();
        sg.name = "Group " + std::to_string(groups.size());
        sg.sampleIds.insert(groups.size());
        sg.color = colourPalette[groups.size() % colourPalette.size()];
        groups.push_back(sg);
    }
}

void TaxonomyManager::setSampleName(int sampleId, std::string name)
{
    if (sampleId >= samples.size())
    {
        extendElementsArrays(sampleId);
    }
    samples[sampleId].name = name;
}
std::string TaxonomyManager::getSampleName(int sampleId)
{
    if (sampleId >= samples.size())
    {
        extendElementsArrays(sampleId);
    }
    return samples[sampleId].name;
}

void TaxonomyManager::setSampleGroup(int sampleId, int groupId)
{
    int highestId = juce::jmax(sampleId, groupId);
    if (highestId >= samples.size())
    {
        extendElementsArrays(highestId);
    }

    // erase the sample from its old group
    groups[samples[sampleId].groupId].sampleIds.erase(sampleId);
    // assign sample to its new group and let the group know
    samples[sampleId].groupId = groupId;
    groups[groupId].sampleIds.insert(sampleId);
}

void TaxonomyManager::copyTaxonomy(int sourceSampleId, int destSampleId)
{
    int highestId = juce::jmax(sourceSampleId, destSampleId);
    if (highestId >= samples.size())
    {
        extendElementsArrays(highestId);
    }

    setSampleName(destSampleId, samples[sourceSampleId].name);
    setSampleGroup(destSampleId, samples[sourceSampleId].groupId);
}

juce::Colour &TaxonomyManager::getSampleColor(int sampleId)
{
    if (sampleId >= samples.size())
    {
        extendElementsArrays(sampleId);
    }

    return groups[samples[sampleId].groupId].color;
}

void TaxonomyManager::setGroupName(int groupId, std::string name)
{
    if (groupId >= samples.size())
    {
        extendElementsArrays(groupId);
    }
    samples[groupId].name = name;
}

void TaxonomyManager::setGroupColor(int groupId, juce::Colour &col)
{
    if (groupId >= samples.size())
    {
        extendElementsArrays(groupId);
    }

    groups[groupId].color = col;
    // TODO: add and set a colourId variable (set it to -1 here)
    // so that users can get their color scheme change applied
    // after reloading a track.
}

void TaxonomyManager::setGroupColor(int groupId, int colorIndex)
{
    if (groupId >= samples.size())
    {
        extendElementsArrays(groupId);
    }

    groups[groupId].color = colourPalette[colorIndex % colourPalette.size()];
}

void TaxonomyManager::disableSample(int sampleId)
{
    if (sampleId >= samples.size())
    {
        return;
    }

    // to disable a sample, we just set it back
    // to its self group and reset its name.
    setSampleGroup(sampleId, sampleId);
    setSampleName(sampleId, "Sample " + std::to_string(sampleId));
}

int TaxonomyManager::getSampleGroup(int id)
{
    if (id >= samples.size())
    {
        return id;
    }

    return samples[id].groupId;
}

std::set<int> TaxonomyManager::getGroupSamples(int gid)
{
    if (gid >= groups.size())
    {
        return std::set<int>();
    }

    return groups[gid].sampleIds;
}