#include "AudioLibraryManager.h"

#include <X11/Xlib.h>
#include <bits/stdc++.h>

#include <climits>
#include <cstddef>
#include <stdexcept>

AudioLibraryManager::AudioLibraryManager(std::string dataFolderAbsolutePath, std::string profile)
{
    profileName = profile;

    // make sure the data folder exists
    juce::File dataFolder = juce::File(juce::String(dataFolderAbsolutePath));
    existsOrCreateFolder(dataFolder);

    // make sure that inside the data folder folder, there is a Library folder
    juce::File libraryFolder = dataFolder.getChildFile("Library");
    existsOrCreateFolder(libraryFolder);

    // make sure that the AudioAccessCounts exists
    juce::File accessCountFolder = libraryFolder.getChildFile("AudioAccessCounts");
    existsOrCreateFolder(accessCountFolder);

    // do we have a count file for this profile ?
    profileCountFile = accessCountFolder.getChildFile(profile + "_profile");

    // initialize the data structure to sort most accessed entries
    minMostUsed = 0;
    nSortedMostUsed = 0;
    topAccessedEntry = nullptr;

    // if it exists load the count of file accesses for the profile
    if (profileCountFile.existsAsFile())
    {
        loadProfileCount();
    }
}

void AudioLibraryManager::existsOrCreateFolder(juce::File folder)
{
    if (!folder.isDirectory())
    {
        // abort with error if it's already a file
        if (folder.exists())
        {
            throw std::runtime_error("Folder is already a file: " + folder.getFullPathName().toStdString());
        }
        juce::Result res = folder.createDirectory();
        if (res.failed())
        {
            throw std::runtime_error("Unable to create data folder " + folder.getFullPathName().toStdString() + ": " +
                                     res.getErrorMessage().toStdString());
        }
    }
}

AudioLibraryManager::~AudioLibraryManager()
{
    // persists profile to disk
    saveProfileCount();
}

void AudioLibraryManager::addAudioLibrary(std::string path)
{
    juce::File dir(path);
    if (!dir.isDirectory())
    {
        throw std::runtime_error("invalid audio library path: " + path);
    }

    // abort if user is trying to add a path that already exists
    if (audioLibrariesRootFiles.find(dir.getFullPathName().toStdString()) != audioLibrariesRootFiles.end())
    {
        throw std::runtime_error("Trying to add a library that already exists");
    }

    searchIndex.addIfNotAlreadyThere(dir);
    juce::String folderName = dir.getFileName();
    audioLibrariesRootFiles.insert(std::pair<std::string, juce::File>(dir.getFullPathName().toStdString(), dir));
    audioLibNamesAndPaths.push_back(
        std::pair<std::string, std::string>(folderName.toStdString(), dir.getFullPathName().toStdString()));
}

std::vector<std::pair<std::string, juce::File>> AudioLibraryManager::getLibraries()
{
    std::vector<std::pair<std::string, juce::File>> response;
    for (size_t i = 0; i < audioLibNamesAndPaths.size(); i++)
    {
        if (audioLibrariesRootFiles.find(audioLibNamesAndPaths[i].second) == audioLibrariesRootFiles.end())
        {
            throw std::runtime_error("unable to locate supposedly existing library");
        }

        juce::File libraryRootFile = audioLibrariesRootFiles[audioLibNamesAndPaths[i].second];

        response.push_back(std::pair<std::string, juce::File>(audioLibNamesAndPaths[i].first, libraryRootFile));
    }
    return response;
}

// NOTE: it's better files received here exists, no check performed
void AudioLibraryManager::countAccess(juce::File &file)
{
    // ignore path with backup count file separator in them
    if (file.getFullPathName().contains(COUNT_FILE_SEPARATOR))
    {
        std::cerr << "A file access count was ignored because path contained "
                     "access count file separator: "
                  << file.getFullPathName().toStdString() << std::endl;
        return;
    }
    // count access
    if (accessCounts.find(file.getFullPathName().toStdString()) != accessCounts.end())
    {
        accessCounts[file.getFullPathName().toStdString()]++;
    }
    else
    {
        accessCounts[file.getFullPathName().toStdString()] = 1;
    }
    // update the top MAX_TOP_ACCESS_SIZE first most accessed entries
    updateMostUsed(file.getFullPathName().toStdString());
    // TODO: once in a while, save it (use timer or count of new entries)
}

void AudioLibraryManager::saveProfileCount()
{
    // save file on disk for the given profile
    juce::FileOutputStream output(profileCountFile);
    if (output.failedToOpen())
    {
        throw std::runtime_error("Unable to open profile access count file");
    }
    if (!output.setPosition(0))
    {
        throw std::runtime_error("Unable to seek in profile access count file");
    }
    auto truncateRes = output.truncate();
    if (truncateRes.failed())
    {
        throw std::runtime_error("Unable to truncate profile access count file: " +
                                 truncateRes.getErrorMessage().toStdString());
    }
    output.setNewLineString("\n");
    // for each count item, save it
    for (auto i : accessCounts)
    {
        output << juce::String(i.first) << COUNT_FILE_SEPARATOR << i.second << "\n";
    }
    output.flush();
    if (output.getStatus().failed())
    {
        // leave it non fatal, just print to stderr
        std::cerr << "An error occured while saving audio access count file: " << output.getStatus().getErrorMessage()
                  << std::endl;
    }
}

void AudioLibraryManager::loadProfileCount()
{
    juce::FileInputStream input(profileCountFile);
    if (input.failedToOpen())
    {
        throw std::runtime_error("Unable to open profile access count file");
    }
    if (!input.setPosition(0))
    {
        throw std::runtime_error("Unable to seek in profile access count file");
    }
    accessCounts.clear();
    std::string content = input.readString().toStdString();
    if (content == "")
    {
        return;
    }
    size_t cursor = 0;
    while (true)
    {
        // stop at end of file
        if (cursor >= content.size())
        {
            break;
        }
        // find the line break and it's we're at it, skip to next char
        size_t nextLineBreak = content.find("\n", cursor);
        if (nextLineBreak == cursor)
        {
            cursor++;
            continue;
        }
        // parse the line
        if (nextLineBreak == std::string::npos)
        {
            nextLineBreak = content.size();
        }
        std::string line = content.substr(cursor, nextLineBreak - cursor);
        size_t sepPos = line.find(COUNT_FILE_SEPARATOR);
        if (sepPos == std::string::npos)
        {
            throw std::runtime_error("A line without a separator was found in the audio access count "
                                     "file");
        }
        std::string linePath = line.substr(0, sepPos);
        std::string countStr = line.substr(sepPos + 1);
        // if the count is not a valid int, we have 0 and it's okay
        int count = atoi(countStr.c_str());
        // see if we can open the file (we will ignore misssing files)
        juce::File fileEntry(linePath);
        if (fileEntry.exists() && count != 0)
        {
            accessCounts.insert(std::pair<std::string, int>(linePath, count));
            updateMostUsed(linePath);
        }
        else
        {
            std::cerr << "File in access count was missing or had zero count: " << linePath << std::endl;
        }
        // go to next line (if above end , will abort at loop beginning)
        cursor = nextLineBreak + 1;
    }
}

void AudioLibraryManager::updateMostUsed(std::string entry)
{
    if (entry == "")
    {
        throw std::runtime_error("received empty entry");
    }

    if (topAccessedEntry == nullptr)
    {
        topAccessedEntry = new SortedAccessCountEntry(entry, nullptr, nullptr);
        return;
    }

    SortedAccessCountEntry *currentEntry = topAccessedEntry;
    SortedAccessCountEntry *previousEntry = nullptr;
    bool entryInserted = false;
    for (size_t i = 0; i < MAX_TOP_ACCESS_SIZE; i++)
    {
        if (!entryInserted)
        {
            // if the current doesn't exist, insert
            if (currentEntry == nullptr)
            {
                previousEntry->next = new SortedAccessCountEntry(entry, previousEntry, nullptr);
                return;
            }
            else
            {
                // if we're already at the right position, do nothing and abort
                if (entry == currentEntry->name)
                {
                    return;
                }
                // if it exists, and count is below new one, insert in it instead
                if (accessCounts[entry] > accessCounts[currentEntry->name])
                {
                    if (previousEntry == nullptr)
                    {
                        currentEntry->previous = new SortedAccessCountEntry(entry, previousEntry, currentEntry);
                        topAccessedEntry = currentEntry->previous;
                        currentEntry = topAccessedEntry;
                    }
                    else
                    {
                        previousEntry->next = new SortedAccessCountEntry(entry, previousEntry, currentEntry);
                        currentEntry->previous = previousEntry->next;
                        currentEntry = previousEntry->next;
                    }
                    entryInserted = true;
                }
            }
            // if we already inserted our entry
        }
        else
        {
            // stop if we reached the end
            if (currentEntry == nullptr)
            {
                return;
            }
            // if we found a duplicate, remove it
            if (entry == currentEntry->name)
            {
                previousEntry->next = currentEntry->next;
                if (currentEntry->next != nullptr)
                {
                    currentEntry->next->previous = previousEntry;
                }
                delete currentEntry;
                currentEntry = previousEntry->next;
                // if we just deleted our lower count value (eg. next smaller doesn't
                // exists)
                i--;
                if (currentEntry == nullptr)
                {
                    break;
                }
            }
        }
        previousEntry = currentEntry;
        currentEntry = previousEntry->next;
    }
}

std::vector<std::string> AudioLibraryManager::getTopEntries(size_t n)
{
    std::vector<std::string> paths;
    paths.reserve(n);
    SortedAccessCountEntry *currentPath = topAccessedEntry;
    size_t i = 0;
    while (i < n && currentPath != nullptr)
    {
        paths.push_back(currentPath->name);
        i++;
        currentPath = currentPath->next;
    }
    return paths;
}

int AudioLibraryManager::getFileAccessCount(std::string path)
{
    auto val = accessCounts.find(path);
    if (val == accessCounts.end())
    {
        return 0;
    }
    else
    {
        return accessCounts[path];
    }
}

std::vector<std::string> AudioLibraryManager::getSearchResults(std::string search, int count)
{
    juce::Array<juce::File> results =
        searchIndex.findChildFiles(juce::File::TypesOfFileToFind::findFilesAndDirectories, true, "*" + search + "*");

    std::vector<std::string> resultLocations;
    resultLocations.reserve(count);
    int max = juce::jmin(results.size(), count);
    for (int i = 0; i < max; i++)
    {
        resultLocations.push_back(results[i].getFullPathName().toStdString());
    }
    return resultLocations;
}