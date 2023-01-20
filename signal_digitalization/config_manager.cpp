#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <filesystem>
#include "json.h"
#include "config_manager.h"
#include "plot_manager.h"
#include <gui.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

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
    ImGui::BeginDisabled(cfg_files.size() == 0 || selected_config >= cfg_files.size());
    if (ImGui::Button("Remove selected", ImVec2(-1.f, 0.f))){
        Remove(cfg_files[selected_config]);
    }
    ImGui::EndDisabled();

#ifdef __EMSCRIPTEN__
    if (ImGui::Button("Import", ImVec2(window_size.x / 4.f - 2.5f * ImGui::GetStyle().ItemSpacing.x, 0.f))) {
        Import();
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(cfg_files.size() == 0 || selected_config >= cfg_files.size());
    if (ImGui::Button("Export selected", ImVec2(window_size.x / 4.f - 2.5f * ImGui::GetStyle().ItemSpacing.x, 0.f))) {
        Export(cfg_files[selected_config]);
    }
    ImGui::EndDisabled();
#endif


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

#ifdef __EMSCRIPTEN__
    RenderImportModalDialog();
#endif
}

void ConfigManager::RefreshConfigs()
{
    cfg_files.clear();

    for (const auto& entry : std::filesystem::directory_iterator("."))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".cfg")
        {
            cfg_files.push_back(entry.path().filename().string());
        }
    }
}

void ConfigManager::Save(const std::string& config_name)
{
    //get file path
    std::string filePath = "";
    try {
        // Get the current working directory
        std::filesystem::path currentPath = std::filesystem::current_path();
        // Append the file name to the current path
        filePath = (currentPath / config_name).string();
        if (!endsWithExtension(filePath, ".cfg")) {
            filePath += ".cfg";
        }
    }
    catch (const std::filesystem::filesystem_error& ex) {
        std::cerr << "Config manager: Error - could not get current directory path: " << ex.what() << std::endl;
        return;
    }

    auto data = plot_manager.GetJsonData();

    try {
        if (std::filesystem::exists(filePath)) {
            if (std::filesystem::is_directory(filePath)) {
                std::cerr << "Error: The given path points to a directory, not a file" << std::endl;
                return;
            }

            if (!(((int)std::filesystem::status(filePath).permissions()) & (int)std::filesystem::perms::owner_write)) {
                std::cerr << "Error: The file is not writable" << std::endl;
                return;
            }
        }
    }
    catch (const std::filesystem::filesystem_error& ex) {
        std::cerr << "Error: Could not check file attributes: " << ex.what() << std::endl;
        return;
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
    try {
        std::filesystem::path currentPath = std::filesystem::current_path();
        if (currentPath.empty())
        {
            std::cerr << "Config manager: Error - could not get current directory path" << std::endl;
            return;
        }

        if (currentPath == "/")
            currentPath = "";

        // Prepend the current path to the file path
        filePath = currentPath.string() + "/" + config_name;
        if (!endsWithExtension(filePath, ".cfg"))
            filePath += ".cfg";
    }
    catch (const std::filesystem::filesystem_error& ex) {
        std::cerr << "Config manager: Error - could not get current directory path: " << ex.what() << std::endl;
        return;
    }

    // Open the file
    std::ifstream file(filePath);
    std::string fileContents = "";

    // Read the file contents into a string
    fileContents.assign((std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());

    LoadData(fileContents);
}

void ConfigManager::LoadData(const std::string& data)
{
    // Parse the JSON data from the string
    nlohmann::json jsonData = nlohmann::json::parse(data);

    plot_manager.ClearAllInputs();
    plot_manager.LoadJsonData(jsonData);

    RefreshConfigs();
}

void ConfigManager::Remove(const std::string& config_name)
{
    std::string filePath = "";
    try {
        std::filesystem::path currentPath = std::filesystem::current_path();
        if (currentPath.empty())
        {
            std::cerr << "Config manager: Error - could not get current directory path" << std::endl;
            return;
        }

        if (currentPath == "/")
            currentPath = "";

        // Prepend the current path to the file path
        filePath = currentPath.string() + "/" + config_name;
        if (!endsWithExtension(filePath, ".cfg"))
            filePath += ".cfg";
    }
    catch (const std::filesystem::filesystem_error& ex) {
        std::cerr << "Config manager: Error - could not get current directory path: " << ex.what() << std::endl;
        return;
    }

    try {
        std::filesystem::remove(filePath);
    }
    catch (const std::filesystem::filesystem_error& ex) {
        std::cerr << "Config manager: Error - could not get remove config: " << ex.what() << std::endl;
        return;
    }
    RefreshConfigs();
}

#ifdef __EMSCRIPTEN__
EM_JS(void, downloadConfigAsFile, (const char* data, int datalen, const char* config_name, int config_name_len), {
    // Create a download link
    var link = document.createElement('a');
    link.download = encodeURIComponent(UTF8ToString(config_name, config_name_len));
    link.href = 'data:text/cfg;charset=utf-8,' + encodeURIComponent(UTF8ToString(data, datalen));

    // Click on the link to trigger the download
    link.click();
    });

void ConfigManager::Export(const std::string& config_name)
{
    std::string filePath = "";
    try {
        std::filesystem::path currentPath = std::filesystem::current_path();
        if (currentPath.empty())
        {
            std::cerr << "Config manager: Error - could not get current directory path" << std::endl;
            return;
        }

        if (currentPath == "/")
            currentPath = "";

        // Prepend the current path to the file path
        filePath = currentPath.string() + "/" + config_name;
        if (!endsWithExtension(filePath, ".cfg"))
            filePath += ".cfg";
    }
    catch (const std::filesystem::filesystem_error& ex) {
        std::cerr << "Config manager: Error - could not get current directory path: " << ex.what() << std::endl;
        return;
    }

    // Open the file
    std::ifstream file(filePath);
    std::string fileContents = "";

    // Read the file contents into a string
    fileContents.assign((std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());

    downloadConfigAsFile(fileContents.c_str(), fileContents.size(), config_name.c_str(), config_name.size());
}

extern "C" {
    EMSCRIPTEN_KEEPALIVE
        void filePickerCallback(const char* fileContents) {
        std::string fileContentsString(fileContents);
        config_manager.temp_import_data = fileContentsString;
        config_manager.open_import_dialog = true;
    }
}

void ConfigManager::Import()
{
    temp_import_data.clear();
    EM_ASM(
        // Define a JavaScript function that opens a file picker dialog and reads the
        // contents of the selected file.
        function openFilePicker() {
        const fileInput = document.createElement('input');
        fileInput.type = 'file';
        fileInput.accept = '.cfg';  // Only allow the user to select .cfg files
        fileInput.onchange = function() {
            const file = fileInput.files[0];
            const reader = new FileReader();
            reader.onload = function() {
                // Call the filePickerCallback function with the contents of the file.
                Module.ccall('filePickerCallback', null, ['string'], [reader.result]);
            };
            reader.readAsText(file);
        };
        fileInput.click();
    }
    // Call the openFilePicker function to open the file picker dialog.
    openFilePicker();
    );
}

void ConfigManager::RenderImportModalDialog()
{
    if (open_import_dialog) {
        ImGui::OpenPopup("Import config");
        open_import_dialog = false;
    }

    if (ImGui::BeginPopupModal("Import config", NULL))
    {
        static char name[256] = "config_name\0";
        ImGui::Text("Input name:");
        ImGui::InputText("##InputNameInput", name, 256);
     
        ImGui::Dummy(ImVec2(300.f, 15.f));

        if (ImGui::Button("Close")) {
            ImGui::CloseCurrentPopup();
            temp_import_data.clear();
        }

        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.31f, 0.98f, 0.48f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.31f, 0.98f, 0.48f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.41f, 0.99f, 0.57f, 0.7f));
        if (ImGui::Button("Import"))
        {
            LoadData(temp_import_data);
            Save(std::string(name));

            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(3);

        ImGui::EndPopup();
    }
}
#endif
