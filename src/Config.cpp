#include "Config.h"

#include <asm-generic/errno-base.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <yaml-cpp/node/node.h>

#include <cerrno>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <string>

std::vector<std::string> Config::mandatoryParameters = {
    "profile", "apiVersion", "AudioLibraryLocations"};

Config::Config(std::string configFilePath) {
  // parsing yaml file in the try, setting as invalid in the catch
  try {
    // if the first two characters are ~/
    // remove tilde and try to get HOME envar
    if (configFilePath.find("~/") == 0) {
      // try to get HOME envar
      std::string homeFolder;
      if (const char* env_p = std::getenv("HOME")) {
        homeFolder = env_p;
      } else {
        throw std::runtime_error("No HOME environement variable set");
      }

      configFilePath.replace(0, 1, homeFolder);
    }

    YAML::Node config = YAML::LoadFile(configFilePath);

    _checkMandatoryParameters(config);

    _checkApiVersion(config);

    _parseAudioLibraryLocations(config);

    _parseProfileName(config);

    _getConfigDirectory(config);

    _getDataDirectory(config);

    _invalid = false;

  } catch (std::runtime_error err) {
    std::cerr << "Error while parsing file: " << err.what() << std::endl;
    _errMsg = err.what();
    _invalid = true;
    throw err;
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

void Config::_getConfigDirectory(YAML::Node& config) {
  std::string path =
      _getProvidedOrDefaultPath(config, "configDirectory", ".kholors");
  _createFolderIfNotExists(path);
  _configDirectoryPath = path;
}

void Config::_getDataDirectory(YAML::Node& config) {
  std::string path =
      _getProvidedOrDefaultPath(config, "dataDirectory", "Kholors");
  _createFolderIfNotExists(path);
  _dataLibraryPath = path;
}

// defaultPathFromHome must not begin with a slash or dot, use raw folder
// name eg. "Data" or ".datadir"
std::string Config::_getProvidedOrDefaultPath(YAML::Node& config,
                                              std::string paramName,
                                              std::string defaultPathFromHome) {
  // if the directory path config is set, use this dir
  if (config[paramName]) {
    // abort if not in the right format or empty
    if (!config[paramName].IsScalar() ||
        config[paramName].as<std::string>() == "") {
      throw std::runtime_error(paramName + std::string("is in invalid format"));
    }
    // return it
    return config[paramName].as<std::string>();
  }

  // use default location
  // if home envar is set, use it
  if (const char* homeDirEnv = std::getenv("HOME")) {
    // make sure it ends with a slash
    std::string homeDir = homeDirEnv;
    if (homeDir.back() != '/') {
      homeDir.push_back('/');
    }
    // append the default folder name
    return homeDir + defaultPathFromHome;
  }

  // if we reach here, HOME or config was not set so we error out
  throw std::runtime_error(
      std::string("Unable to get path for config ") + paramName +
      std::string(" or to find HOME envar to use default location"));
}

void Config::_createFolderIfNotExists(std::string path) {
  struct stat st;
  if (stat(path.c_str(), &st) == -1) {
    // if the folder does not exists, create it
    if (errno == ENOENT) {
      // use perimissions of parent dir and abort on error
      if (mkdir(path.c_str(), 0755) == -1) {
        throw std::runtime_error(
            std::string("Unable to create directory at path: ") + path +
            std::string(": mkdir failed with error: ") +
            std::string(strerror(errno)));
      }
      return;
    }
    throw std::runtime_error(std::string("OS stat call failed with error: ") +
                             strerror(errno));
  }
  // if we reach here, something was found, we check it's a dir and writable
  // before returning
  bool isDirectory = S_ISDIR(st.st_mode);
  if (!isDirectory) {
    throw std::runtime_error(std::string("Expected a folder for path: ") +
                             path);
  }
  // BUG: this tests if the folder is writeable by real UID and not effective
  // UID
  if (access(path.c_str(), W_OK) != 0) {
    throw std::runtime_error(
        std::string("Folder path is not writable by real UID: ") + path);
  }
}