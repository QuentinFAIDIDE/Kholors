#include <iostream>

#include "../src/Config.h"

int testConfig1() {
  Config cfg1("./test/test01.yaml");

  if (cfg1.isInvalid() == true) {
    std::cout << "First test config was invalid: " << cfg1.getErrMessage()
              << std::endl;
    return 1;
  }

  if (cfg1.getProfileName() != "default") {
    std::cout << "Unable to parse profile name" << std::endl;
    return 1;
  }

  if (cfg1.getNumAudioLibs() != 3) {
    std::cout << "Unable to parse number of audio libraries" << std::endl;
    return 1;
  }

  if (cfg1.getAudioLibName(0) != "Samples") {
    std::cout << "Unable to parse audio lib name 1" << std::endl;
    return 1;
  }
  if (cfg1.getAudioLibName(1) != "Dingo") {
    std::cout << "Unable to parse audio lib name 2" << std::endl;
    return 1;
  }
  if (cfg1.getAudioLibName(2) != "folder") {
    std::cout << cfg1.getAudioLibName(2) << std::endl;
    std::cout << "Unable to parse audio lib name 3" << std::endl;
    return 1;
  }

  if (cfg1.getAudioLibPath(0) != "/home/folder/samples") {
    std::cout << "Unable to parse audio lib path 1" << std::endl;
    return 1;
  }
  if (cfg1.getAudioLibPath(1) != "/path/to/my/dingo") {
    std::cout << "Unable to parse audio lib path 2" << std::endl;
    return 1;
  }
  if (cfg1.getAudioLibPath(2) != "/my/unnamed/folder") {
    std::cout << "Unable to parse audio lib path 3" << std::endl;
    return 1;
  }

  if (cfg1.audioLibIgnoreCount(0) != false) {
    std::cout << "Unable to parse audio ignoreUseCount 1" << std::endl;
    return 1;
  }
  if (cfg1.audioLibIgnoreCount(1) != true) {
    std::cout << "Unable to parse audio ignoreUseCount 2" << std::endl;
    return 1;
  }
  if (cfg1.audioLibIgnoreCount(2) != false) {
    std::cout << "Unable to parse audio ignoreUseCount 3" << std::endl;
    return 1;
  }

  return 0;
}

int testConfig2() {
  // second config is invalid (garbage)
  Config cfg2("./test/test02.yaml");
  if (cfg2.isInvalid() == false) {
    std::cout << "Second test config valid" << std::endl;
    return 1;
  }

  // this one has invalid empty library
  Config cfg3("./test/test03.yaml");
  if (cfg3.isInvalid() == false) {
    std::cout << "Third test config valid" << std::endl;
    return 1;
  }

  // this one has missing required key "profile"
  Config cfg4("./test/test04.yaml");
  if (cfg3.isInvalid() == false) {
    std::cout << "Fourth test config valid" << std::endl;
    return 1;
  }

  return 0;
}

int main() {
  try {
    int failure = 0;

    failure = testConfig1();
    if (failure != 0) return 1;

    failure = testConfig2();
    if (failure != 0) return 1;

  } catch (const std::runtime_error& err) {
    std::cerr << "exception during test: " << err.what() << std::endl;
    return 1;

  } catch (std::runtime_error* err) {
    std::cerr << "exception during test: " << err->what() << std::endl;
    return 1;
  }

  return 0;
}