#ifndef DEF_AUDIO_SAMPLE_LIB_HPP
#define DEF_AUDIO_SAMPLE_LIB_HPP

#include <map>
#include <string>
#include <vector>

#include "juce_core/juce_core.h"

#define COUNT_FILE_SEPARATOR "?"
#define MAX_TOP_ACCESS_SIZE 2000

class SortedAccessCountEntry {
 public:
  SortedAccessCountEntry(std::string n, SortedAccessCountEntry *p,
                         SortedAccessCountEntry *ne)
      : name(n), previous(p), next(ne){};
  std::string name;
  SortedAccessCountEntry *previous;
  SortedAccessCountEntry *next;
};

class AudioLibraryManager {
 public:
  AudioLibraryManager(std::string dataFolderPath, std::string profile);
  ~AudioLibraryManager();
  void addAudioLibrary(std::string path);
  void countAccess(juce::File &);
  std::vector<std::pair<std::string, juce::File>> getLibraries();
  std::vector<std::string> getTopEntries(size_t n = MAX_TOP_ACCESS_SIZE);
  int getFileAccessCount(std::string);

 private:
  std::map<std::string, juce::File> _audioLibrariesRootFiles;
  std::vector<std::pair<std::string, std::string>> _audioLibNamesAndPaths;
  juce::FileSearchPath _searchIndex;
  std::map<std::string, int> _accessCounts;
  std::string _profileName;
  juce::File _profileCountFile;
  int _minMostUsed;
  int _nSortedMostUsed;

  SortedAccessCountEntry *_topAccessedEntry;

  void _loadProfileCount();
  void _saveProfileCount();
  void _existsOrCreateFolder(juce::File);
  void _updateMostUsed(std::string);
};

#endif  // DEF_AUDIO_SAMPLE_LIB_HPP