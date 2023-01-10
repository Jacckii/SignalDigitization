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
//  Git: https://github.com/epezent/implo
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

bool g_done = false;
std::unique_ptr<GUI> gui;

void main_loop() {
    if (gui->beginFrame()) {
        static ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::Begin("Demonstrace digitalizace signalu", NULL, flags);

        auto window_width = ImGui::GetCurrentWindow()->Size.x - (ImGui::GetStyle().ItemSpacing.x * 4.f);
        ImGui::Columns(3, "##layout", false);
        {
            ImGui::SetColumnWidth(0, window_width * 0.20f);
            ImGui::BeginChild("Input", ImVec2(0.f, 0.f), true);
            ImGui::Text("Input:");

            if (ImGui::Button("+", ImVec2(-1.f, 0.f))) {
                plot_manager.OpenAddInputDialog();
            }
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
