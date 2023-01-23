//Bakaláøská práce téma: Aplikace pro demonstraci digitalizace signálu 
//Autor: Filip Nejedlý
//Git: https://github.com/Jacckii

//Libraries used:
//ImGui: 
//  Autor: Ocornut
//  Git: https://github.com/ocornut/imgui
//  Description: GUI Framework
//ImPlot:
//  Autor: epezent
//  Git: https://github.com/epezent/implot
//  Description: Plot widgets for ImGui
//ImGuiFileDialog:
//  Autor: aiekick
//  Git: https://github.com/aiekick/ImGuiFileDialog
//  Description: File Dialog for Dear ImGui
//MagicEnum:
//  Autor: Neargye
//  Git: https://github.com/Neargye/magic_enum
//  Description: Static reflection for enums (to string, from string, iteration) 
//               for modern C++, work with any enum type without any macro or 
//               boilerplate code
//exprtk:
//  Autor: Arash Partow
//  Web: http://www.partow.net/programming/exprtk/
//  Git: https://github.com/ArashPartow/exprtk
//  Description: C++ Mathematical Expression Parsing And Evaluation Library
//nlohmann::json:
//  Autor: nlohmann
//  Git: https://github.com/nlohmann/json
//  Description: JSON for Modern C++
//EMSCRIPTEN
//  Autor: Alon Zakai
//  Git: https://github.com/emscripten-core/emscripten
//  Description: Emscripten compiles C and C++ to WebAssembly using LLVM and Binaryen. 
//               Emscripten output can run on the Web, in Node.js, and in wasm runtimes.

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include <string> 
#include <vector>
#include <iostream>
#include <gui.h>
#include <implot.h>
#include <chrono>
#include <imgui_internal.h>
#include "plot_manager.h"
#include "config_manager.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <GLES3/gl3.h>
#endif

bool show_app_style_editor = false;
bool show_app_plot_style_editor = false;
bool show_Imgui_about = false;
bool show_app_about = false;
bool g_done = false;
std::unique_ptr<GUI> gui;

//by not including this window and its' informations will break the MITM license!
static void ShowAboutWindow(bool* p_open)
{
    static bool show_config_info = false;
    static bool show_config_info_old = false;
    ImGui::SetNextWindowSize(ImVec2(700.f, 200.f), ImGuiCond_Appearing);
    if (show_config_info != show_config_info_old) {
        ImGui::SetNextWindowSize(ImVec2(700.f, show_config_info ? 450.f : 200.f));
        show_config_info_old = show_config_info;
    }
    if (!ImGui::Begin("About Signal digitalization application", p_open))
    {
        ImGui::End();
        return;
    }

    ImGui::Text("Coded by Filip Nejedly at UTB.");    
    ImGui::Text("This project is licensed under the MIT license!");

    ImGui::Checkbox("Show licence", &show_config_info);
    if (show_config_info) {
        ImGuiIO& io = ImGui::GetIO();
        ImGuiStyle& style = ImGui::GetStyle();

#ifndef __EMSCRIPTEN__
        bool copy_to_clipboard = ImGui::Button("Copy to clipboard");
#endif
        ImVec2 child_size = ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 18);
        ImGui::BeginChildFrame(ImGui::GetID("license"), child_size, ImGuiWindowFlags_NoMove);
#ifndef __EMSCRIPTEN__
        if (copy_to_clipboard)
        {
            ImGui::LogToClipboard();
            ImGui::LogText("```\n"); // Back quotes will make text appears without formatting when pasting on GitHub
        }
#endif

        ImGui::TextWrapped("MIT License\r\n");
        ImGui::NewLine();
        ImGui::TextWrapped("Copyright(c) 2023 Jacckii\r\n");
        ImGui::NewLine();
        ImGui::TextWrapped(R"(Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:)");
        ImGui::NewLine();
        ImGui::TextWrapped(R"(The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.)");
        ImGui::NewLine();
        ImGui::TextWrapped(R"(THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.)");

#ifndef __EMSCRIPTEN__
        if (copy_to_clipboard)
        {
            ImGui::LogText("\n```\n");
            ImGui::LogFinish();
        }
#endif
        ImGui::EndChildFrame();
    }
    ImGui::End();
}

void main_loop() {
    if (gui->beginFrame()) {
        if (show_app_style_editor)
        {
            ImGui::Begin("Style Editor (ImGui)", &show_app_style_editor);
            ImGui::ShowStyleEditor();
            ImGui::End();
        }

        if (show_app_plot_style_editor)
        {
            ImGui::SetNextWindowSize(ImVec2(415, 762), ImGuiCond_Appearing);
            ImGui::Begin("Style Editor (ImPlot)", &show_app_plot_style_editor);
            ImPlot::ShowStyleEditor();
            ImGui::End();
        }

        if (show_Imgui_about)
            ImGui::ShowAboutWindow(&show_Imgui_about);

        if (show_app_about)
            ShowAboutWindow(&show_app_about);

        static ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_MenuBar;
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        if (ImGui::Begin("Demonstrace digitalizace signalu", NULL, flags)) {

            if (ImGui::BeginMenuBar())
            {
                if (ImGui::BeginMenu("Tools"))
                {
                    ImGui::MenuItem("Style Editor ImGui", NULL, &show_app_style_editor);
                    ImGui::MenuItem("Style Editor ImPlot", NULL, &show_app_plot_style_editor);
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("About"))
                {
                    ImGui::MenuItem("About Dear ImGui", NULL, &show_Imgui_about);
                    ImGui::MenuItem("About App", NULL, &show_app_about);
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }

            auto window_width = ImGui::GetCurrentWindow()->Size.x - (ImGui::GetStyle().ItemSpacing.x * 4.f);
            ImGui::Columns(3, "##layout", false);
            {
                ImGui::SetColumnWidth(0, window_width * 0.20f);
                ImGui::BeginChild("Input", ImVec2(0.f, 0.f), true);
                ImGui::Text("Input:");

                if (ImGui::Button("+", ImVec2(-1.f, 0.f))) {
                    plot_manager.OpenAddInputDialog();
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Add analog signal");

                plot_manager.RenderAddInputDialog();
                ImGui::BeginChild("InputCards");
                plot_manager.RenderInputCards();
                ImGui::EndChild();

                ImGui::EndChild();
            }
            ImGui::NextColumn();
            {
                ImGui::SetColumnWidth(1, window_width * 0.60f);
                ImGui::BeginChild("Plot", ImVec2(0.f, 0.f), true);
                {
                    plot_manager.RenderMainPlot();
                }
                ImGui::EndChild();
                plot_manager.TickData();
                ImGui::BeginChild("Plot settings", ImVec2(0.f, 0.f), true);
                {
                    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
                    if (ImGui::BeginTabBar("Settings tabs", tab_bar_flags))
                    {
                        if (ImGui::BeginTabItem("Settings"))
                        {
                            plot_manager.RenderMainPlotSettings();
                            ImGui::EndTabItem();
                        }
                        if (ImGui::BeginTabItem("Output##tab"))
                        {
                            plot_manager.RenderTextOutput();
                            ImGui::EndTabItem();
                        }
                        if (ImGui::BeginTabItem("Configs"))
                        {
                            config_manager.RenderConfigTab();
                            ImGui::EndTabItem();
                        }
                        ImGui::EndTabBar();
                    }
                }
                ImGui::EndChild();

                ImGui::EndChild();
            }
            ImGui::NextColumn();
            {
                ImGui::SetColumnWidth(2, window_width * 0.20f);
                ImGui::BeginChild("DA settings", ImVec2(0.f, 0.f), true);
                ImGui::Text("Digitalization settings:");

                plot_manager.RenderDigitalizationOtions();

                ImGui::EndChild();
            }
        }
        ImGui::End();
        gui->endFrame();
    }
}

int main(int argc, char* argv[]) {
    gui.reset();
    gui = std::make_unique<GUI>(L"Demonstrace digitalizace signalu", 900, 580);

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(main_loop, 0, 1);
#else
    while (!g_done)
    {
        try {
            main_loop();
#ifdef DX10_GUI
            if (gui->getMsg().message == WM_QUIT)
                break;
#endif
        }
        catch (const std::exception& ex) {
            printf("\n\n!!!\n%s\n!!\n", ex.what());
        }
    }
#endif
    return 0;
}
