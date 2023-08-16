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
#include <regex>
#include <stdexcept>
#include <string>

std::vector<std::string> Config::mandatoryParameters = {"name", "mail", "profile", "apiVersion",
                                                        "AudioLibraryLocations"};

Config::Config()
{
    invalid = true;
    errMsg = "Not initialized";
}

Config::Config(std::string configFilePath)
{
    // parsing yaml file in the try, setting as invalid in the catch
    try
    {
        // if the first two characters are ~/
        // remove tilde and try to get HOME envar
        if (configFilePath.find("~/") == 0)
        {
            // try to get HOME envar
            std::string homeFolder;
            if (const char *env_p = std::getenv("HOME"))
            {
                homeFolder = env_p;
            }
            else
            {
                throw std::runtime_error("No HOME environement variable set");
            }

            configFilePath.replace(0, 1, homeFolder);
        }

        YAML::Node config = YAML::LoadFile(configFilePath);

        checkMandatoryParameters(config);

        checkApiVersion(config);

        parseAudioLibraryLocations(config);

        parseProfileName(config);

        parseName(config);

        parseMail(config);

        parseConfigDirectory(config);

        parseDataDirectory(config);

        parseBufferSize(config);

        invalid = false;
    }
    catch (std::runtime_error err)
    {
        std::cerr << "Error while parsing file: " << err.what() << std::endl;
        errMsg = err.what();
        invalid = true;
    }
}

void Config::checkMandatoryParameters(YAML::Node &config)
{
    for (size_t i = 0; i < mandatoryParameters.size(); i++)
    {
        if (!config[mandatoryParameters[i]])
        {
            throw std::runtime_error(std::string("missing mandatory config ") + mandatoryParameters[i]);
        }
    }
}

void Config::checkApiVersion(YAML::Node &config)
{
    std::string apiVersion = config["apiVersion"].as<std::string>();
    if (apiVersion != "v0")
    {
        throw std::runtime_error(std::string("Unsupported config api version: ") + apiVersion);
    }
}

void Config::parseAudioLibraryLocations(YAML::Node &config)
{
    if (!config["AudioLibraryLocations"].IsSequence())
    {
        throw std::runtime_error("Audio library locations must be a list");
    }

    if (config["AudioLibraryLocations"].size() == 0)
    {
        throw std::runtime_error("Audio library locations can't be empty");
    }

    for (size_t i = 0; i < config["AudioLibraryLocations"].size(); i++)
    {
        YAML::Node audioLibLocEntry = config["AudioLibraryLocations"][i];

        if (!audioLibLocEntry.IsMap())
        {
            throw std::runtime_error("Audio library entry must be a map");
        }

        parseAudioLibLocationPath(audioLibLocEntry);
        parseAudioLibLocationName(audioLibLocEntry);
        parseAudioLibLocationIgnoreCount(audioLibLocEntry);
    }
}

void Config::parseAudioLibLocationPath(YAML::Node &audioLibLocEntry)
{
    checkIfFieldScalarAndExists(audioLibLocEntry, "path");

    // remove trailing / if they exists as a convention
    std::string trimmedPathName = audioLibLocEntry["path"].as<std::string>();
    if (trimmedPathName.back() == '/')
    {
        trimmedPathName.pop_back();
    }
    audioLibPaths.push_back(trimmedPathName);
}

void Config::parseAudioLibLocationName(YAML::Node &audioLibLocEntry)
{
    std::string trimmedPathName = audioLibLocEntry["path"].as<std::string>();
    // abort if path is empty or in root folder
    if (trimmedPathName.find("/") == std::string::npos)
    {
        throw std::runtime_error("Unable to use a library directory on root folder");
    }

    // if the audio lib name exists, use it, if not, use folder name as
    // convention
    std::string folderName = trimmedPathName.substr(trimmedPathName.find_last_of("/") + 1,
                                                    trimmedPathName.size() - 1 - trimmedPathName.find_last_of("/"));
    bool hasName = true;
    try
    {
        checkIfFieldScalarAndExists(audioLibLocEntry, "name");
    }
    catch (std::exception)
    {
        hasName = false;
    }
    if (hasName)
    {
        folderName = audioLibLocEntry["name"].as<std::string>();
    }
    audioLibNames.push_back(folderName);
}

void Config::parseAudioLibLocationIgnoreCount(YAML::Node &audioLibLocEntry)
{
    try
    {
        checkIfFieldScalarAndExists(audioLibLocEntry, "ignoreUseCount");
    }
    catch (std::exception)
    {
        audioLibIgnoreCounts.push_back(false);
        return;
    }
    audioLibIgnoreCounts.push_back(audioLibLocEntry["ignoreUseCount"].as<bool>());
}

void Config::checkIfFieldScalarAndExists(YAML::Node &node, std::string field)
{
    if (!node[field])
    {
        throw std::runtime_error(std::string("Audio library entry must have a ") + field + std::string(" field"));
    }
    if (!node[field].IsScalar())
    {
        throw std::runtime_error(std::string("Audio library entry field ") + field + std::string(" must be a scalar"));
    }
}

void Config::parseProfileName(YAML::Node &config)
{
    if (!config["profile"] || !config["profile"].IsScalar())
    {
        profile = "default";
    }
    else
    {
        profile = config["profile"].as<std::string>();
    }
}

void Config::parseName(YAML::Node &config)
{
    if (!config["name"] || !config["name"].IsScalar())
    {
        throw std::runtime_error(std::string("Missing or invalid name field"));
    }

    name = config["name"].as<std::string>();
    if (!std::regex_match(name, std::regex(NAME_REGEX)))
    {
        throw std::runtime_error(std::string("Unrecognised name format"));
    }
}

std::string Config::getName() const
{
    return name;
}

std::string Config::getMail() const
{
    return mail;
}

void Config::parseMail(YAML::Node &config)
{
    if (!config["mail"] || !config["mail"].IsScalar())
    {
        throw std::runtime_error(std::string("Missing or invalid mail field"));
    }

    mail = config["mail"].as<std::string>();
    if (!std::regex_match(mail, std::regex(MAIL_REGEX)))
    {
        throw std::runtime_error(std::string("Unrecognised mail format"));
    }
}

bool Config::isInvalid() const
{
    return invalid;
}

std::string Config::getProfileName() const
{
    return profile;
}

int Config::getNumAudioLibs() const
{
    return audioLibNames.size();
}

std::string Config::getAudioLibName(unsigned long i) const
{
    if (i >= audioLibNames.size())
    {
        throw std::runtime_error("index out of bounds");
    }
    return audioLibNames[i];
}

std::string Config::getAudioLibPath(unsigned long i) const
{
    if (i >= audioLibNames.size())
    {
        throw std::runtime_error("index out of bounds");
    }
    return audioLibPaths[i];
}

bool Config::audioLibIgnoreCount(unsigned long i) const
{
    if (i >= audioLibNames.size())
    {
        throw std::runtime_error("index out of bounds");
    }
    return audioLibIgnoreCounts[i];
}

std::string Config::getErrMessage() const
{
    return errMsg;
}

void Config::parseConfigDirectory(YAML::Node &config)
{
    std::string path = findConfigPathOrProvideDefault(config, "configDirectory", ".kholors");
    createFolderIfNotExists(path);
    configDirectoryPath = path;
}

void Config::parseDataDirectory(YAML::Node &config)
{
    std::string path = findConfigPathOrProvideDefault(config, "dataDirectory", "Kholors");
    createFolderIfNotExists(path);
    dataLibraryPath = path;
}

// defaultPathFromHome must not begin with a slash or dot, use raw folder
// name eg. "Data" or ".datadir"
std::string Config::findConfigPathOrProvideDefault(YAML::Node &config, std::string paramName,
                                                   std::string defaultPathFromHome)
{
    // if the directory path config is set, use this dir
    if (config[paramName])
    {
        // abort if not in the right format or empty
        if (!config[paramName].IsScalar() || config[paramName].as<std::string>() == "")
        {
            throw std::runtime_error(paramName + std::string("is in invalid format"));
        }
        // return it
        return config[paramName].as<std::string>();
    }

    // use default location
    // if home envar is set, use it
    if (const char *homeDirEnv = std::getenv("HOME"))
    {
        // make sure it ends with a slash
        std::string homeDir = homeDirEnv;
        if (homeDir.back() != '/')
        {
            homeDir.push_back('/');
        }
        // append the default folder name
        return homeDir + defaultPathFromHome;
    }

    // if we reach here, HOME or config was not set so we error out
    throw std::runtime_error(std::string("Unable to get path for config ") + paramName +
                             std::string(" or to find HOME envar to use default location"));
}

void Config::createFolderIfNotExists(std::string path)
{
    struct stat st;
    if (stat(path.c_str(), &st) == -1)
    {
        // if the folder does not exists, create it
        if (errno == ENOENT)
        {
            // use perimissions of parent dir and abort on error
            if (mkdir(path.c_str(), 0755) == -1)
            {
                throw std::runtime_error(std::string("Unable to create directory at path: ") + path +
                                         std::string(": mkdir failed with error: ") + std::string(strerror(errno)));
            }
            return;
        }
        throw std::runtime_error(std::string("OS stat call failed with error: ") + strerror(errno));
    }
    // if we reach here, something was found, we check it's a dir and writable
    // before returning
    bool isDirectory = S_ISDIR(st.st_mode);
    if (!isDirectory)
    {
        throw std::runtime_error(std::string("Expected a folder for path: ") + path);
    }
    // BUG: this tests if the folder is writeable by real UID and not effective
    // UID
    if (access(path.c_str(), W_OK) != 0)
    {
        throw std::runtime_error(std::string("Folder path is not writable by real UID: ") + path);
    }
}

std::string Config::getDataFolderPath() const
{
    return dataLibraryPath;
};

void Config::parseBufferSize(YAML::Node &n)
{
    bufferSize = 0;

    if (n["AudioSettings"] && n["AudioSettings"].IsMap())
    {
        YAML::Node audioParams = n["AudioSettings"];
        if (audioParams["bufferSize"] && audioParams["bufferSize"].IsScalar())
        {
            bufferSize = audioParams["bufferSize"].as<int>();

            // abort if buffer size is invalid
            if (bufferSize <= 0)
            {
                throw std::runtime_error("invalid buffer size");
            }
        }
    }
}

int Config::getBufferSize() const
{
    return bufferSize;
}