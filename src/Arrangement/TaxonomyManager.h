#ifndef DEF_TAXONOMY_MANAGER
#define DEF_TAXONOMY_MANAGER

#include "Marshalable.h"
#include <juce_gui_extra/juce_gui_extra.h>

#include <string>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

/**
 * @brief      This class describes taxonomy data for a sample group.
 */
struct SampleGroup
{
    int groupId;
    std::string name;
    std::set<int> sampleIds;
    juce::Colour color;
};

/**
 * @brief      This class describes taxonomy data for a sample.
 */
struct SampleMetadata
{
    int sampleId;
    int groupId;
    std::string name;
};

/**
 * @brief      helpers to convert local structs back and forth with json.
 */
void to_json(json &j, const SampleGroup &sg);

/**
 * @brief      helpers to convert local structs back and forth with json.
 */
void from_json(const json &j, SampleGroup &sg);

/**
 * @brief      helpers to convert local structs back and forth with json.
 */
void to_json(json &j, const SampleMetadata &sg);

/**
 * @brief      helpers to convert local structs back and forth with json.
 */
void from_json(const json &j, SampleMetadata &sg);

class TaxonomyManager : public Marshalable
{
  public:
    TaxonomyManager();

    void setSampleName(int sampleId, std::string name);
    void setSampleGroup(int sampleId, int groupId);

    std::string getSampleName(int sampleId);

    void copyTaxonomy(int sourceSampleId, int destSampleId);

    juce::Colour &getSampleColor(int sampleId);

    void setGroupName(int groupId, std::string name);
    void setGroupColor(int groupId, juce::Colour &c);
    void setGroupColor(int groupId, int colorId);
    int getSampleGroup(int sampleId);
    void disableSample(int sampleId);

    std::set<int> getGroupSamples(int groupId);

    /**
     * @brief      Resets the object.
     */
    void reset();

    /**
     * @brief      Dump a json of the taxonomy.
     *
     * @return     stringified json.
     */
    std::string marshal() override;

    /**
     * @brief      Load the taxonomy from an already existing file.
     *
     * @param      s     the json text to parse with the taxonomy previously
     *                   saved with marshall function.
     */
    void unmarshal(std::string &s) override;

  private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TaxonomyManager)

    std::vector<SampleMetadata> samples;
    std::vector<SampleGroup> groups;

    void extendElementsArrays(int);
};

#endif // DEF_GROUP_MANAGER