#ifndef DEF_AUDIO_SAMPLE_LIB_HPP
#define DEF_AUDIO_SAMPLE_LIB_HPP

#include <map>
#include <string>
#include <vector>

#include "juce_core/juce_core.h"

#define COUNT_FILE_SEPARATOR "?"
#define MAX_TOP_ACCESS_SIZE 2000

/**
 * A double linked list that helps at sorting sample entries in library dynamically
 * as well as in bulk quickly based on their access count.
 */
class SortedAccessCountEntry
{
  public:
    SortedAccessCountEntry(std::string n, SortedAccessCountEntry *p, SortedAccessCountEntry *ne)
        : name(n), previous(p), next(ne){};
    std::string name;
    SortedAccessCountEntry *previous;
    SortedAccessCountEntry *next;
};

/**
 * Manages the different locations for audio samples, allow to search
 * and order them by most used.
 */
class AudioLibraryManager
{
  public:
    /**
     * constructor based on user profile name and the location of the
     * software data folder where the counts are kept.
     */
    AudioLibraryManager(std::string dataFolderPath, std::string profile);

    /**
     * Destructor (responsible for freeing all these dangerous linked lists)
     */
    ~AudioLibraryManager();

    /**
     * Add an audio library.
     * @param path its path on disk.
     */
    void addAudioLibrary(std::string path);

    /**
     * Tells the library that a file was accessed.
     */
    void countAccess(juce::File &);

    /**
     * Get the list of libraries and their corresponding file object.
     */
    std::vector<std::pair<std::string, juce::File>> getLibraries();

    /**
     * Return the most accessed locations.
     */
    std::vector<std::string> getTopEntries(size_t n = MAX_TOP_ACCESS_SIZE);

    /**
     * Get some search results from the library.
     */
    std::vector<std::string> getSearchResults(std::string search, int count);

    /**
     * Returns how many times this file (or directory) has been used in the app.
     * @param  std::string file path
     * @return             app import count
     */
    int getFileAccessCount(std::string);

  private:
    std::map<std::string, juce::File> audioLibrariesRootFiles;
    std::vector<std::pair<std::string, std::string>> audioLibNamesAndPaths;
    juce::FileSearchPath searchIndex;
    std::map<std::string, int> accessCounts;
    std::string profileName;
    juce::File profileCountFile;
    int minMostUsed;
    int nSortedMostUsed;

    SortedAccessCountEntry *topAccessedEntry;

    void loadProfileCount();
    void saveProfileCount();
    void existsOrCreateFolder(juce::File);
    void updateMostUsed(std::string);
};

#endif // DEF_AUDIO_SAMPLE_LIB_HPP