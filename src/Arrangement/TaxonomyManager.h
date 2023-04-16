#ifndef DEF_TAXONOMY_MANAGER
#define DEF_TAXONOMY_MANAGER

#include <juce_gui_extra/juce_gui_extra.h>

#include <string>

class SampleGroup {
 public:
  int groupId;
  std::string name;
  std::set<int> sampleIds;
  juce::Colour color;
};

class SampleMetadata {
 public:
  int sampleId;
  int groupId;
  std::string name;
};

class TaxonomyManager {
 public:
  TaxonomyManager();

  void setSampleName(int sampleId, std::string name);
  void setSampleGroup(int sampleId, int groupId);

  std::string getSampleName(int sampleId);

  void copyTaxonomy(int sourceSampleId, int destSampleId);

  juce::Colour& getSampleColor(int sampleId);

  void setGroupName(int groupId, std::string name);
  void setGroupColor(int groupId, juce::Colour& c);
  void setGroupColor(int groupId, int colorId);

  void disableSample(int sampleId);

 private:
  std::vector<SampleMetadata> samples;
  std::vector<SampleGroup> groups;

  void extendElementsArrays(int);
};

#endif  // DEF_GROUP_MANAGER