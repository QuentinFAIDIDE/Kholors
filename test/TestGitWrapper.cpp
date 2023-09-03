#include <filesystem>
#include <fstream>
#include <iostream>

#include "../src/Arrangement/GitWrapper.h"

int main()
{

    GitWrapper git;

    // create a new folder
    std::filesystem::create_directory("./test-git-repo");

    // do a git init
    std::string fullpath = std::filesystem::current_path().generic_string() + "/test-git-repo";

    // add a test file with content 0
    std::ofstream testFile("./test-git-repo/tf.txt");
    testFile << "0";
    testFile.close();

    // commit the changes
    git.setWorkingDirectory("./test-git-repo");
    git.init();
    git.add("tf.txt");
    git.commit("initial commit");

    // write 1 into the file
    testFile = std::ofstream("./test-git-repo/tf.txt");
    testFile << "1";
    testFile.close();

    // do a git hard reset
    git.resetCurrentChanges();

    // assert that the file has indeed been reset to 0
    std::ifstream testReadFile("./test-git-repo/tf.txt");
    std::string testFileText;
    getline(testReadFile, testFileText);
    testReadFile.close();

    if (testFileText != "0")
    {
        std::cerr << "File content does not match expected state after hard reset: " << testFileText << std::endl;
        return 1;
    }

    // delete the test folder
    std::filesystem::remove_all("./test-git-repo");

    return 0;
}