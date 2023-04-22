#include "DefaultConfigPath.h"

std::string parseConfigPath(const juce::String &args)
{
    std::string configpath = args.fromFirstOccurrenceOf("--config ", false, false).toStdString();

    if (configpath == "")
    {
        return std::string(DEFAULT_CONFIG_FILE);
    }

    // remove eventual spaces at beginning
    while (configpath.find(" ") == 0)
    {
        configpath = configpath.substr(1, configpath.size() - 1);
    }
    // only keep the part before the last whitespace
    auto firstWhitespaceLoc = configpath.find(' ');
    if (firstWhitespaceLoc != std::string::npos)
    {
        configpath = configpath.substr(0, firstWhitespaceLoc);
    }

    if (configpath == "")
    {
        return std::string(DEFAULT_CONFIG_FILE);
    }
    else
    {
        return configpath;
    }
}