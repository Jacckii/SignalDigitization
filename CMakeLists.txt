cmake_minimum_required (VERSION 3.0)

project(signal_digitalization)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Wextra -Wno-missing-braces")

if(${EMSCRIPTEN})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -s USE_SDL=2 -std=c++1z")
    #file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/web/index.html DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
else()
    find_package(SDL2 REQUIRED)
    find_package(OpenGL REQUIRED)
endif()

include_directories(src)
include_directories(SYSTEM external ${SDL2_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIRS} dependencies/includes GUI/includes)

add_executable(index
	GUI/includes/gui.h
	GUI/includes/gui_conf.h
	GUI/includes/imconfig.h
	GUI/includes/imgui.h
	GUI/includes/imgui_impl_opengl3.h
	GUI/includes/imgui_impl_opengl3_loader.h
	GUI/includes/imgui_impl_sdl.h
	GUI/includes/imgui_internal.h
	GUI/includes/ImGuiFileDialog.h
	GUI/includes/ImGuiFileDialogConfig.h
	GUI/includes/implot.h
	GUI/includes/implot_internal.h
	GUI/includes/imstb_rectpack.h
	GUI/includes/imstb_textedit.h
	GUI/includes/imstb_truetype.h
	GUI/src/gui.cpp
	GUI/src/imgui.cpp
	GUI/src/imgui_draw.cpp
	GUI/src/imgui_impl_opengl3.cpp
	GUI/src/imgui_impl_sdl.cpp
	GUI/src/imgui_tables.cpp
	GUI/src/imgui_widgets.cpp
	GUI/src/ImGuiFileDialog.cpp
	GUI/src/implot.cpp
	GUI/src/implot_items.cpp
    signal_digitalization/structs.h
    signal_digitalization/plot_manager.h
    signal_digitalization/plot_manager.cpp
    signal_digitalization/noise_generator.h
    signal_digitalization/main.cpp
    signal_digitalization/json.h
    signal_digitalization/config_manager.h
    signal_digitalization/config_manager.cpp
)

set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++17")

target_link_libraries(index ${SDL2_LIBRARIES} ${OPENGL_LIBRARIES}) 