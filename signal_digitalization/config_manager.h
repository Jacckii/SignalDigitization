#pragma once
#ifndef __EMSCRIPTEN__
#include <dirent.h>
#include <string>
#include <vector>
#include <iostream>

//TODO: add Linux, OSX support

class ConfigManager
{
public:
    ConfigManager();
    ~ConfigManager() {}

    void RenderConfigTab();
private:
    // Refresh the list of existing config files
    void RefreshConfigs();

    // Save into an existing config to a file or create new
    void Save(const std::string& config_name);

    // Load an existing config from a file
    void Load(const std::string& config_name);

    // List of existing config files
    std::vector<std::string> cfg_files;
};

extern ConfigManager config_manager;
#endif