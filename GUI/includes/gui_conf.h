#pragma once

#ifdef __EMSCRIPTEN__
	#define SDL2_GUI 
	#define WEBASSEMBLY_GUI
#else
	#define DX10_GUI
	#define WIN32_GUI
#endif

#if !defined(DX10_GUI) && !defined(SDL2_GUI)
#error "You must select library for rendering!"
#endif

#if !defined(WIN32_GUI) && !defined(WEBASSEMBLY_GUI)
#error "You must select platform for the app!"
#endif