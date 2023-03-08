#include "AudioLibraryManager.h"

#include <X11/Xlib.h>
#include <bits/stdc++.h>

#include <climits>
#include <cstddef>
#include <stdexcept>

AudioLibraryManager::AudioLibraryManager(std::string dataFolderAbsolutePath,
                                         std::string profile) {
  _profileName = profile;

  // make sure the data folder exists
  juce::File dataFolder = juce::File(juce::String(dataFolderAbsolutePath));
  _existsOrCreateFolder(dataFolder);

  // make sure that inside the data folder folder, there is a Library folder
  juce::File libraryFolder = dataFolder.getChildFile("Library");
  _existsOrCreateFolder(libraryFolder);

  // make sure that the AudioAccessCounts exists
  juce::File accessCountFolder =
      libraryFolder.getChildFile("AudioAccessCounts");
  _existsOrCreateFolder(accessCountFolder);

  // do we have a count file for this profile ?
  _profileCountFile = accessCountFolder.getChildFile(profile + "_profile");

  // initialize the data structure to sort most accessed entries
  _minMostUsed = 0;
  _nSortedMostUsed = 0;
  _topAccessedEntry = nullptr;

  // if it exists load the count of file accesses for the profile
  if (_profileCountFile.existsAsFile()) {
    _loadProfileCount();
  }
}

void AudioLibraryManager::_existsOrCreateFolder(juce::File folder) {
  if (!folder.isDirectory()) {
    // abort with error if it's already a file
    if (folder.exists()) {
      throw std::runtime_error("Folder is already a file: " +
                               folder.getFullPathName().toStdString());
    }
    juce::Result res = folder.createDirectory();
    if (res.failed()) {
      throw std::runtime_error("Unable to create data folder " +
                               folder.getFullPathName().toStdString() + ": " +
                               res.getErrorMessage().toStdString());
    }
  }
}

AudioLibraryManager::~AudioLibraryManager() {
  // free all SortedAccessCountEntry
  SortedAccessCountEntry* currentEntry;
  SortedAccessCountEntry* previousEntry;
  currentEntry = _topAccessedEntry;
  while (currentEntry != nullptr) {
    previousEntry = currentEntry;
    currentEntry = previousEntry->next;
    _nSortedMostUsed--;
    delete (previousEntry);
  }
  // persists profile to disk
  _saveProfileCount();
}

void AudioLibraryManager::addAudioLibrary(std::string path) {
  juce::File dir(path);
  if (!dir.isDirectory()) {
    throw std::runtime_error("invalid audio library path: " + path);
  }

  // abort if user is trying to add a path that already exists
  if (_audioLibrariesRootFiles.find(dir.getFullPathName().toStdString()) !=
      _audioLibrariesRootFiles.end()) {
    throw std::runtime_error("Trying to add a library that already exists");
  }

  _searchIndex.addIfNotAlreadyThere(dir);
  juce::String folderName = dir.getFileName();
  _audioLibrariesRootFiles.insert(std::pair<std::string, juce::File>(
      dir.getFullPathName().toStdString(), dir));
  _audioLibNamesAndPaths.push_back(std::pair<std::string, std::string>(
      folderName.toStdString(), dir.getFullPathName().toStdString()));
}

std::vector<std::pair<std::string, juce::File>>
AudioLibraryManager::getLibraries() {
  std::vector<std::pair<std::string, juce::File>> response;
  for (size_t i = 0; i < _audioLibNamesAndPaths.size(); i++) {
    if (_audioLibrariesRootFiles.find(_audioLibNamesAndPaths[i].second) ==
        _audioLibrariesRootFiles.end()) {
      throw std::runtime_error("unable to locate supposedly existing library");
    }

    juce::File libraryRootFile =
        _audioLibrariesRootFiles[_audioLibNamesAndPaths[i].second];

    response.push_back(std::pair<std::string, juce::File>(
        _audioLibNamesAndPaths[i].first, libraryRootFile));
  }
  return response;
}

// NOTE: it's better files received here exists, no check performed
void AudioLibraryManager::countAccess(juce::File& file) {
  // ignore path with backup count file separator in them
  if (file.getFullPathName().contains(COUNT_FILE_SEPARATOR)) {
    std::cerr << "A file access count was ignored because path contained "
                 "access count file separator: "
              << file.getFullPathName().toStdString() << std::endl;
    return;
  }
  // count access
  if (_accessCounts.find(file.getFullPathName().toStdString()) !=
      _accessCounts.end()) {
    _accessCounts[file.getFullPathName().toStdString()]++;
  } else {
    _accessCounts[file.getFullPathName().toStdString()] = 1;
  }
  // update the top MAX_TOP_ACCESS_SIZE first most accessed entries
  _updateMostUsed(file.getFullPathName().toStdString());
  // TODO: once in a while, save it (use timer or count of new entries)
}

void AudioLibraryManager::_saveProfileCount() {
  // save file on disk for the given profile
  juce::FileOutputStream output(_profileCountFile);
  if (output.failedToOpen()) {
    throw std::runtime_error("Unable to open profile access count file");
  }
  if (!output.setPosition(0)) {
    throw std::runtime_error("Unable to seek in profile access count file");
  }
  auto truncateRes = output.truncate();
  if (truncateRes.failed()) {
    throw std::runtime_error("Unable to truncate profile access count file: " +
                             truncateRes.getErrorMessage().toStdString());
  }
  output.setNewLineString("\n");
  // for each count item, save it
  for (auto i : _accessCounts) {
    output << juce::String(i.first) << COUNT_FILE_SEPARATOR << i.second << "\n";
  }
  output.flush();
  if (output.getStatus().failed()) {
    // leave it non fatal, just print to stderr
    std::cerr << "An error occured while saving audio access count file: "
              << output.getStatus().getErrorMessage() << std::endl;
  }
}

void AudioLibraryManager::_loadProfileCount() {
  juce::FileInputStream input(_profileCountFile);
  if (input.failedToOpen()) {
    throw std::runtime_error("Unable to open profile access count file");
  }
  if (!input.setPosition(0)) {
    throw std::runtime_error("Unable to seek in profile access count file");
  }
  _accessCounts.clear();
  std::string content = input.readString().toStdString();
  if (content == "") {
    return;
  }
  size_t cursor = 0;
  while (true) {
    // stop at end of file
    if (cursor >= content.size()) {
      break;
    }
    // find the line break and it's we're at it, skip to next char
    size_t nextLineBreak = content.find("\n", cursor);
    if (nextLineBreak == cursor) {
      cursor++;
      continue;
    }
    // parse the line
    if (nextLineBreak == std::string::npos) {
      nextLineBreak = content.size();
    }
    std::string line = content.substr(cursor, nextLineBreak - cursor);
    size_t sepPos = line.find(COUNT_FILE_SEPARATOR);
    if (sepPos == std::string::npos) {
      throw std::runtime_error(
          "A line without a separator was found in the audio access count "
          "file");
    }
    std::string linePath = line.substr(0, sepPos);
    std::string countStr = line.substr(sepPos + 1);
    // if the count is not a valid int, we have 0 and it's okay
    int count = atoi(countStr.c_str());
    // see if we can open the file (we will ignore misssing files)
    juce::File fileEntry(linePath);
    if (fileEntry.exists() && count != 0) {
      _accessCounts.insert(std::pair<std::string, int>(linePath, count));
      _updateMostUsed(linePath);
    } else {
      std::cerr << "File in access count was missing or had zero count: "
                << linePath << std::endl;
    }
    // go to next line (if above end , will abort at loop beginning)
    cursor = nextLineBreak + 1;
  }
}

void AudioLibraryManager::_updateMostUsed(std::string entry) {
  if (entry == "") {
    throw std::runtime_error("received empty entry");
  }

  if (_topAccessedEntry == nullptr) {
    _topAccessedEntry = new SortedAccessCountEntry(entry, nullptr, nullptr);
    return;
  }

  SortedAccessCountEntry* currentEntry = _topAccessedEntry;
  SortedAccessCountEntry* previousEntry = nullptr;
  bool entryInserted = false;
  for (size_t i = 0; i < MAX_TOP_ACCESS_SIZE; i++) {
    if (!entryInserted) {
      // if the current doesn't exist, insert
      if (currentEntry == nullptr) {
        previousEntry->next =
            new SortedAccessCountEntry(entry, previousEntry, nullptr);
        return;
      } else {
        // if we're already at the right position, do nothing and abort
        if (entry == currentEntry->name) {
          return;
        }
        // if it exists, and count is below new one, insert in it instead
        if (_accessCounts[entry] > _accessCounts[currentEntry->name]) {
          if (previousEntry == nullptr) {
            currentEntry->previous =
                new SortedAccessCountEntry(entry, previousEntry, currentEntry);
            _topAccessedEntry = currentEntry->previous;
            currentEntry = _topAccessedEntry;
          } else {
            previousEntry->next =
                new SortedAccessCountEntry(entry, previousEntry, currentEntry);
            currentEntry->previous = previousEntry->next;
            currentEntry = previousEntry->next;
          }
          entryInserted = true;
        }
      }
      // if we already inserted our entry
    } else {
      // stop if we reached the end
      if (currentEntry == nullptr) {
        return;
      }
      // if we found a duplicate, remove it
      if (entry == currentEntry->name) {
        previousEntry->next = currentEntry->next;
        if (currentEntry->next != nullptr) {
          currentEntry->next->previous = previousEntry;
        }
        delete currentEntry;
        currentEntry = previousEntry->next;
        // if we just deleted our lower count value (eg. next smaller doesn't
        // exists)
        i--;
        if (currentEntry == nullptr) {
          break;
        }
      }
    }
    previousEntry = currentEntry;
    currentEntry = previousEntry->next;
  }
}

std::vector<std::string> AudioLibraryManager::getTopEntries(size_t n) {
  std::vector<std::string> paths;
  paths.reserve(n);
  SortedAccessCountEntry* currentPath = _topAccessedEntry;
  size_t i = 0;
  while (i < n && currentPath != nullptr) {
    paths.push_back(currentPath->name);
    i++;
    currentPath = currentPath->next;
  }
  return paths;
}

int AudioLibraryManager::getFileAccessCount(std::string path) {
  auto val = _accessCounts.find(path);
  if (val == _accessCounts.end()) {
    return 0;
  } else {
    return _accessCounts[path];
  }
}