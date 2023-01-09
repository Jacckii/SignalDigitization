// dear imgui: Platform Backend for SDL2
// This needs to be used along with a Renderer (e.g. DirectX11, OpenGL3, Vulkan..)
// (Info: SDL2 is a cross-platform general purpose library for handling windows, inputs, graphics context creation, etc.)

// Implemented features:
//  [X] Platform: Clipboard support.
//  [X] Platform: Keyboard support. Since 1.87 we are using the io.AddKeyEvent() function. Pass ImGuiKey values to all key functions e.g. ImGui::IsKeyPressed(ImGuiKey_Space). [Legacy SDL_SCANCODE_* values will also be supported unless IMGUI_DISABLE_OBSOLETE_KEYIO is set]
//  [X] Platform: Gamepad support. Enabled with 'io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad'.
//  [X] Platform: Mouse cursor shape and visibility. Disable with 'io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange'.
// Missing features:
//  [ ] Platform: SDL2 handling of IME under Windows appears to be broken and it explicitly disable the regular Windows IME. You can restore Windows IME by compiling SDL with SDL_DISABLE_WINDOWS_IME.

// You can use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#pragma once
#include "gui_conf.h"
#ifdef SDL2_GUI
// ImGui SDL2 binding with OpenGL
// https://github.com/ocornut/imgui

struct SDL_Window;
typedef union SDL_Event SDL_Event;

IMGUI_API bool        ImGui_ImplSdl_Init(SDL_Window* window);
IMGUI_API void        ImGui_ImplSdl_Shutdown();
IMGUI_API void        ImGui_ImplSdl_NewFrame(SDL_Window* window);
IMGUI_API bool        ImGui_ImplSdl_ProcessEvent(SDL_Event* event);

// Use if you want to reset your rendering device without losing ImGui state.
IMGUI_API void        ImGui_ImplSdl_InvalidateDeviceObjects();
IMGUI_API bool        ImGui_ImplSdl_CreateDeviceObjects();
#endif