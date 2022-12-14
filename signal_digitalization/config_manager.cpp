#include <iostream>
#include <fstream>
#include <string>
#include <dirent.h>
#include <direct.h>
#include "json.h"
#include "config_manager.h"
#include "plot_manager.h"
#include <gui.h>

ConfigManager config_manager;

// Function that checks if a string ends with a specific extension
static bool endsWithExtension(const std::string& str, const std::string& ext)
{
    // Get the length of the string and the extension
    size_t strLength = str.length();
    size_t extLength = ext.length();

    // Check if the string is long enough to contain the extension
    if (strLength < extLength)
    {
        return false;
    }

    // Check if the string ends with the given extension
    return str.compare(strLength - extLength, extLength, ext) == 0;
}

ConfigManager::ConfigManager()
{
    RefreshConfigs();
}

void ConfigManager::RenderConfigTab()
{
    ImGui::Text("Configs:");
    ImGui::Columns(2);
    static size_t selected_config = 0;
    auto window_size = ImGui::GetWindowSize();
    
    static char config_name[127] = "";
    ImGui::BeginDisabled(cfg_files.size() == 0 || selected_config >= cfg_files.size());
    if (ImGui::Button("Save", ImVec2(window_size.x / 4.f - 2.5f * ImGui::GetStyle().ItemSpacing.x, 0.f))) {
        Save(cfg_files[selected_config]);
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(cfg_files.size() == 0 || selected_config >= cfg_files.size());
    if (ImGui::Button("Load", ImVec2(window_size.x / 4.f - 2.5f * ImGui::GetStyle().ItemSpacing.x, 0.f))) {
        Load(cfg_files[selected_config]);
    }
    ImGui::EndDisabled();
    ImGui::BeginDisabled(strlen(config_name) < 2);
    if (ImGui::Button("Add new", ImVec2(91.f, 0.f))) {
        Save(std::string(config_name));
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::PushItemWidth((window_size.x / 2.f) - 100.f - ImGui::GetStyle().ItemSpacing.x * 2);
    ImGui::InputText("##new_config_name", config_name, 127);

    ImGui::NextColumn();

    ImGui::PushItemWidth(-1.f);
    if (ImGui::BeginListBox("##config_list", ImVec2(0, window_size.y - ImGui::GetStyle().WindowPadding.y * 7)))
    {
        for (size_t n = 0; n < cfg_files.size(); n++)
        {
            const bool is_selected = (selected_config == n);
            if (ImGui::Selectable(cfg_files[n].c_str(), is_selected))
                selected_config = n;

            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndListBox();
    }
    ImGui::PopItemWidth();
}

void ConfigManager::RefreshConfigs()
{
    cfg_files.clear();
    // Use the opendir function to open the current directory
    DIR* dir = opendir(".");

    // Check if the directory was successfully opened
    if (dir == nullptr) {
        // Print an error message and return if the directory could not be opened
        std::cerr << "Config manager: Error - could not open directory" << std::endl;
        return;
    }

    // Use the readdir function to read the entries in the directory one by one
    dirent* entry = readdir(dir);
    while (entry != nullptr) {
        // Check if the entry is a regular file (not a directory, etc.)
        // and if it has the ".cfg" extension
        if (entry->d_type == DT_REG && endsWithExtension(entry->d_name, ".cfg")) {
            // If the entry is a ".cfg" file, add its name to the vector
            cfg_files.push_back(entry->d_name);
        }

        // Read the next entry in the directory
        entry = readdir(dir);
    }

    // Close the directory using the closedir function
    closedir(dir);
}

void ConfigManager::Save(const std::string& config_name)
{
    //get file path
    std::string filePath = "";
    char currentPath[FILENAME_MAX];
    if (_getcwd(currentPath, sizeof(currentPath)) != nullptr)
    {
        // Prepend the current path to the file path
        filePath = std::string(currentPath) + "\\" + config_name;
        if (!endsWithExtension(filePath, ".cfg"))
            filePath += ".cfg";
    }
    else {
        std::cerr << "Config manager: Error - could not get current directory path" << std::endl;
        return;
    }

    auto data = plot_manager.GetJsonData();

    //check if exists and if can be written
    DWORD fileAttributes = GetFileAttributesA(filePath.c_str());
    if (fileAttributes != INVALID_FILE_ATTRIBUTES) {
        if (fileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            std::cerr << "Error: The given path points to a directory, not a file" << std::endl;
            return;
        }

        if (fileAttributes & FILE_ATTRIBUTE_READONLY) {
            std::cerr << "The file is not read-only and can be written to" << std::endl;
            return;
        }
    }

    // Use the `dump()` function to convert the JSON data to a string
    std::string json_str = data.dump();

    // Use the `ofstream` class to write the string to a file
    try {
        std::ofstream out_file(filePath);
        out_file << json_str;
        out_file.close();
    }
    catch (const std::exception& ex) {
        std::cerr << "Error while writing to a file: " << ex.what() << std::endl;
    }

    RefreshConfigs();
}

void ConfigManager::Load(const std::string& config_name)
{
    std::string filePath = "";
    char currentPath[FILENAME_MAX];
    if (_getcwd(currentPath, sizeof(currentPath)) != nullptr)
    {
        // Prepend the current path to the file path
        filePath = std::string(currentPath) + "\\" + config_name;
        if (!endsWithExtension(filePath, ".cfg"))
            filePath += ".cfg";
    }
    else {
        std::cerr << "Config manager: Error - could not get current directory path" << std::endl;
        return;
    }

    // Open the file
    std::ifstream file(filePath);

    // Read the file contents into a string
    std::stringstream fileContents;
    fileContents << file.rdbuf();

    // Parse the JSON data from the string
    nlohmann::json jsonData = nlohmann::json::parse(fileContents.str());

    plot_manager.ClearAllInputs();
    plot_manager.LoadJsonData(jsonData);

    RefreshConfigs();
}
