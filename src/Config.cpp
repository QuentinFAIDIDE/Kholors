#include "Config.h"

#include <yaml-cpp/node/node.h>

#include <exception>
#include <iostream>
#include <stdexcept>

std::vector<std::string> Config::mandatoryParameters = {
    "profile", "apiVersion", "AudioLibraryLocations"};

Config::Config(std::string configFilePath) {
  // parsing yaml file in the try, setting as invalid in the catch
  try {
    YAML::Node config = YAML::LoadFile(configFilePath);

    _checkMandatoryParameters(config);

    _checkApiVersion(config);

    _parseAudioLibraryLocations(config);

    _parseProfileName(config);

    _invalid = false;

  } catch (std::runtime_error err) {
    std::cerr << "Error while parsing file: " << err.what() << std::endl;
    _errMsg = err.what();
    _invalid = true;
  }
}

void Config::_checkMandatoryParameters(YAML::Node& config) {
  for (size_t i = 0; i < mandatoryParameters.size(); i++) {
    if (!config[mandatoryParameters[i]]) {
      throw std::runtime_error(std::string("missing mandatory config ") +
                               mandatoryParameters[i]);
    }
  }
}

void Config::_checkApiVersion(YAML::Node& config) {
  std::string apiVersion = config["apiVersion"].as<std::string>();
  if (apiVersion != "v0") {
    throw std::runtime_error(std::string("Unsupported config api version: ") +
                             apiVersion);
  }
}

void Config::_parseAudioLibraryLocations(YAML::Node& config) {
  if (!config["AudioLibraryLocations"].IsSequence()) {
    throw std::runtime_error("Audio library locations must be a list");
  }

  if (config["AudioLibraryLocations"].size() == 0) {
    throw std::runtime_error("Audio library locations can't be empty");
  }

  for (size_t i = 0; i < config["AudioLibraryLocations"].size(); i++) {
    YAML::Node audioLibLocEntry = config["AudioLibraryLocations"][i];

    if (!audioLibLocEntry.IsMap()) {
      throw std::runtime_error("Audio library entry must be a map");
    }

    _parseAudioLibLocationPath(audioLibLocEntry);
    _parseAudioLibLocationName(audioLibLocEntry);
    _parseAudioLibLocationIgnoreCount(audioLibLocEntry);
  }
}

void Config::_parseAudioLibLocationPath(YAML::Node& audioLibLocEntry) {
  _checkIfFieldScalarAndExists(audioLibLocEntry, "path");

  // remove trailing / if they exists as a convention
  std::string trimmedPathName = audioLibLocEntry["path"].as<std::string>();
  if (trimmedPathName.back() == '/') {
    trimmedPathName.pop_back();
  }
  _audioLibPaths.push_back(trimmedPathName);
}

void Config::_parseAudioLibLocationName(YAML::Node& audioLibLocEntry) {
  std::string trimmedPathName = audioLibLocEntry["path"].as<std::string>();
  // abort if path is empty or in root folder
  if (trimmedPathName.find("/") == std::string::npos) {
    throw std::runtime_error(
        "Unable to use a library directory on root folder");
  }

  // if the audio lib name exists, use it, if not, use folder name as
  // convention
  std::string folderName = trimmedPathName.substr(
      trimmedPathName.find_last_of("/") + 1,
      trimmedPathName.size() - 1 - trimmedPathName.find_last_of("/"));
  bool hasName = true;
  try {
    _checkIfFieldScalarAndExists(audioLibLocEntry, "name");
  } catch (std::exception) {
    hasName = false;
  }
  if (hasName) {
    folderName = audioLibLocEntry["name"].as<std::string>();
  }
  _audioLibNames.push_back(folderName);
}

void Config::_parseAudioLibLocationIgnoreCount(YAML::Node& audioLibLocEntry) {
  try {
    _checkIfFieldScalarAndExists(audioLibLocEntry, "ignoreUseCount");
  } catch (std::exception) {
    _audioLibIgnoreCounts.push_back(false);
    return;
  }
  _audioLibIgnoreCounts.push_back(
      audioLibLocEntry["ignoreUseCount"].as<bool>());
}

void Config::_checkIfFieldScalarAndExists(YAML::Node& node, std::string field) {
  if (!node[field]) {
    throw std::runtime_error(std::string("Audio library entry must have a ") +
                             field + std::string(" field"));
  }
  if (!node[field].IsScalar()) {
    throw std::runtime_error(std::string("Audio library entry field ") + field +
                             std::string(" must be a scalar"));
  }
}

void Config::_parseProfileName(YAML::Node& config) {
  if (!config["profile"] || !config["profile"].IsScalar()) {
    _profile = "default";
  } else {
    _profile = config["profile"].as<std::string>();
  }
}

bool Config::isInvalid() const { return _invalid; }

std::string Config::getProfileName() const { return _profile; }

int Config::getNumAudioLibs() const { return _audioLibNames.size(); }

std::string Config::getAudioLibName(unsigned long i) const {
  if (i >= _audioLibNames.size()) {
    throw std::runtime_error("index out of bounds");
  }
  return _audioLibNames[i];
}

std::string Config::getAudioLibPath(unsigned long i) const {
  if (i >= _audioLibNames.size()) {
    throw std::runtime_error("index out of bounds");
  }
  return _audioLibPaths[i];
}

bool Config::audioLibIgnoreCount(unsigned long i) const {
  if (i >= _audioLibNames.size()) {
    throw std::runtime_error("index out of bounds");
  }
  return _audioLibIgnoreCounts[i];
}

std::string Config::getErrMessage() const { return _errMsg; }