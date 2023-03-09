#ifndef DEFAULT_CONFIG_PATH_HPP
#define DEFAULT_CONFIG_PATH_HPP

#include <juce_gui_extra/juce_gui_extra.h>

#define DEFAULT_CONFIG_FILE "~/.kholors/default.yaml"

std::string parseConfigPath(const juce::String& args);

#endif  // DEFAULT_CONFIG_PATH_HPP