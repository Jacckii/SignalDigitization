#pragma once
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

    // Load config from std::string data
    void LoadData(const std::string& data);

    // Remove an existing config
    void Remove(const std::string& config_name);

#ifdef __EMSCRIPTEN__
    // Export an existing config to download
    void Export(const std::string& config_name);

    // Import an existing config to Web assembly file system
    void Import();
public:
    std::string temp_import_data;
    bool open_import_dialog = false;
private:
    // import finish modal dialog
    void RenderImportModalDialog();

    
#endif

    // List of existing config files
    std::vector<std::string> cfg_files;
};

extern ConfigManager config_manager;