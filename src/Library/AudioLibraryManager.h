#ifndef DEF_AUDIO_SAMPLE_LIB_HPP
#define DEF_AUDIO_SAMPLE_LIB_HPP

#include "../Config.h"
#include "AudioLibraryFile.h"

class AudioLibraryManager {
 public:
  AudioLibraryManager(Config&);
  ~AudioLibraryManager();

  // return top  most accessed entries
  std::vector<AudioLibraryFile> GetTopAccessedEntries(size_t start,
                                                      size_t stop) const;
  // return all files or subfolders in folder
  std::vector<AudioLibraryFile> GetEntries(AudioLibraryFile path) const;
  // increment access count of folder and its parents
  void countAccess(AudioLibraryFile path);
  // check if there are updates in the tree, and return the files for which
  // there were changes
  std::vector<AudioLibraryFile> refreshIndexing();
};

#endif  // DEF_AUDIO_SAMPLE_LIB_HPP