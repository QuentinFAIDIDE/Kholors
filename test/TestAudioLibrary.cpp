#include <cstdlib>
#include <numeric>
#include <string>

#include "../src/Library/AudioLibraryManager.h"

int main()
{
    // remove eventual existing libcount file
    juce::File f("../test/TestAudioLib1Data/Library/AudioAccessCounts/default_profile");
    f.deleteFile();
    // try to load the audio library in the test folder and test top 100 is right
    AudioLibraryManager *lib = new AudioLibraryManager("../test/TestAudioLib1Data/", "default");
    juce::File a("./a");
    juce::File b("./b");
    juce::File c("./c");
    lib->countAccess(a);
    lib->countAccess(a);
    lib->countAccess(a);
    lib->countAccess(b);
    lib->countAccess(b);
    lib->countAccess(c);
    std::vector<std::string> ent = lib->getTopEntries();
    if (ent.size() != 3)
    {
        std::cerr << "Wrong size: " << ent.size() << std::endl;
        return 1;
    }
    if (ent[0] != a.getFullPathName())
    {
        std::cerr << "first sorted entry is not a: " << ent[0] << std::endl;
        return 1;
    }
    if (ent[1] != b.getFullPathName())
    {
        std::cerr << "first sorted entry is not b: " << ent[1] << std::endl;
        return 1;
    }
    if (ent[2] != c.getFullPathName())
    {
        std::cerr << "first sorted entry is not c: " << ent[2] << std::endl;
        return 1;
    }

    // create files so they are actually loaded
    a.create();
    b.create();
    c.create();

    // try to unload and load lib
    delete lib;
    lib = new AudioLibraryManager("../test/TestAudioLib1Data/", "default");

    ent = lib->getTopEntries();
    if (ent.size() != 3)
    {
        std::cerr << "Wrong size after save: " << ent.size() << std::endl;
        return 1;
    }
    if (ent[0] != a.getFullPathName())
    {
        std::cerr << "first sorted entry is not a after save: " << ent[0] << std::endl;
        return 1;
    }
    if (ent[1] != b.getFullPathName())
    {
        std::cerr << "first sorted entry is not b after save: " << ent[1] << std::endl;
        return 1;
    }
    if (ent[2] != c.getFullPathName())
    {
        std::cerr << "first sorted entry is not c after save: " << ent[2] << std::endl;
        return 1;
    }

    a.deleteFile();
    b.deleteFile();
    c.deleteFile();

    delete lib;

    // now lets generate fuzzy data to test the ordering !
    f.deleteFile();
    // try to load the audio library in the test folder and test top 100 is right
    lib = new AudioLibraryManager("../test/TestAudioLib1Data/", "default");

    // create 4000 files
    std::vector<juce::File> testFiles;
    for (size_t i = 0; i < 2000; i++)
    {
        testFiles.push_back(juce::File("../test/TestAudioLib1Data/Library/TestSamples/" + std::to_string(i)));
    }
    // create a predictable random pattern
    juce::Random randgen(4562);

    for (size_t i = 0; i < 10000; i++)
    {
        size_t randpick = (size_t)(randgen.nextInt()) % 2000;
        lib->countAccess(testFiles[randpick]);
    }

    // get first entries
    std::vector<std::string> ent2 = lib->getTopEntries();
    // detect duplicate and wrong ordering
    std::set<std::string> entriesFound;

    int lastCount = lib->getFileAccessCount(ent2[0]);
    for (size_t i = 0; i < ent2.size(); i++)
    {
        if (entriesFound.find(ent2[i]) != entriesFound.end())
        {
            std::cerr << "a key had duplicate value: " << ent2[i] << std::endl;
            return 1;
        }
        entriesFound.insert(ent2[i]);
        if (lib->getFileAccessCount(ent2[i]) > lastCount)
        {
            std::cerr << "a key had higher count than previous one" << std::endl;
            return 1;
        }
        lastCount = lib->getFileAccessCount(ent2[i]);
    }

    delete lib;

    return 0;
}