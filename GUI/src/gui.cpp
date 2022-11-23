//Simple static GUI library
//Libraries used:
//ImGui: 
//  Autor: Ocornut
//  Git: https://github.com/ocornut/imgui
//  Description: GUI Framework
//ImPlot:
//  Autor: epezent
//  Git: https://github.com/epezent/implo
//  Description: Plot widgets for ImGui

#include "..\includes\gui.h"
#include "..\includes\implot.h"
#include <d3d10_1.h>
#include <d3d10.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <tchar.h>
#include <stdint.h>
#include <vector>
#include <sstream>

MSG msg;
ImVec4 clear_color;
// Data
ID3D10Device*            g_pd3dDevice;
IDXGISwapChain*          g_pSwapChain;
ID3D10RenderTargetView*  g_mainRenderTargetView;
ImFont* defFont;

HWND hwnd;
WNDCLASSEX wc;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool CreateDeviceD3D(HWND hWnd)
{
	// Setup swap chain
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 2;
	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	UINT createDeviceFlags = 0;
	//createDeviceFlags |= D3D10_CREATE_DEVICE_DEBUG;
	if (D3D10CreateDeviceAndSwapChain(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, D3D10_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice) != S_OK)
		return false;

	CreateRenderTarget();
	return true;
}

void CleanupDeviceD3D()
{
	CleanupRenderTarget();
	if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
	if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

void CreateRenderTarget()
{
	ID3D10Texture2D* pBackBuffer;
	g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
	pBackBuffer->Release();
}

void CleanupRenderTarget()
{
	if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}
// Win32 message handler
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_SIZE:
		if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
		{
			CleanupRenderTarget();
			g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
			CreateRenderTarget();
		}
		return 0;
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;
	}
	return ::DefWindowProc(hWnd, msg, wParam, lParam);
}

GUI::GUI()
{
	Init(L"Dear ImGui - FN", 800, 800);
}

GUI::GUI(const wchar_t* windowName, int w, int h)
{
	Init(windowName, w, h);
}

static std::string GetSystemFontFile(const std::string& faceName) {

	static const LPCWSTR fontRegistryPath = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts";
	HKEY hKey;
	LONG result;
	std::string sFaceName(faceName.begin(), faceName.end());

	// Open Windows font registry key
	result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, fontRegistryPath, 0, KEY_READ, &hKey);
	if (result != ERROR_SUCCESS) {
		return "";
	}

	DWORD maxValueNameSize, maxValueDataSize;
	result = RegQueryInfoKey(hKey, 0, 0, 0, 0, 0, 0, 0, &maxValueNameSize, &maxValueDataSize, 0, 0);
	if (result != ERROR_SUCCESS) {
		return "";
	}

	DWORD valueIndex = 0;
	LPSTR valueName = new CHAR[maxValueNameSize];
	LPBYTE valueData = new BYTE[maxValueDataSize];
	DWORD valueNameSize, valueDataSize, valueType;
	std::string wsFontFile;

	// Look for a matching font name
	do {

		wsFontFile.clear();
		valueDataSize = maxValueDataSize;
		valueNameSize = maxValueNameSize;

		result = RegEnumValueA(hKey, valueIndex, valueName, &valueNameSize, 0, &valueType, valueData, &valueDataSize);

		valueIndex++;

		if (result != ERROR_SUCCESS || valueType != REG_SZ) {
			continue;
		}

		std::string wsValueName(valueName, valueNameSize);

		// Found a match
		if (_strnicmp(sFaceName.c_str(), wsValueName.c_str(), sFaceName.length()) == 0) {

			wsFontFile.assign((LPSTR)valueData, valueDataSize);
			break;
		}
	} while (result != ERROR_NO_MORE_ITEMS);

	delete[] valueName;
	delete[] valueData;

	RegCloseKey(hKey);

	if (wsFontFile.empty()) {
		return "";
	}

	// Build full font file path
	CHAR winDir[MAX_PATH];
	HRESULT ret = GetWindowsDirectoryA(winDir, MAX_PATH);

	if (ret == 0)
	{
		MessageBoxW(GetActiveWindow(), L"Error while getting fonts!", L"Error", MB_OK | MB_ICONQUESTION);
	}

	std::stringstream ss;
	ss << winDir << "\\Fonts\\" << wsFontFile;
	wsFontFile = ss.str();

	return std::string(wsFontFile.begin(), wsFontFile.end());
}

void GUI::Init(const wchar_t* windowName, int w, int h) {
	g_pd3dDevice = NULL;
	g_pSwapChain = NULL;
	g_mainRenderTargetView = NULL;
	clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	ZeroMemory(&msg, sizeof(msg));
	// Create application window
	wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, windowName, NULL };
	::RegisterClassEx(&wc);
	hwnd = ::CreateWindow(wc.lpszClassName, windowName, WS_OVERLAPPEDWINDOW, 100, 100, w, h, NULL, NULL, wc.hInstance, NULL);

	// Initialize Direct3D
	if (!CreateDeviceD3D(hwnd))
	{
		CleanupDeviceD3D();
		::UnregisterClass(wc.lpszClassName, wc.hInstance);
		return;
	}

	// Show the window
	::ShowWindow(hwnd, SW_SHOWDEFAULT);
	::UpdateWindow(hwnd);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImPlot::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	colorSettings();
	ImPlot::GetStyle().AntiAliasedLines = true;
	//ImGui::StyleColorsDark();

	// Setup Platform/Renderer bindings
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX10_Init(g_pd3dDevice);

	static const ImWchar ranges[] =
	{
		0x0020, 0xFB00
	};

	std::string arial = GetSystemFontFile("Arial");
	ImFontConfig mainFontCfg = ImFontConfig();
	mainFontCfg.OversampleH = 3;
	mainFontCfg.OversampleV = 3;
	defFont = ImGui::GetIO().Fonts->AddFontFromFileTTF(arial.c_str(), 14.0f, &mainFontCfg, ranges);
	ImGui::GetIO().FontDefault = defFont;
}

GUI::~GUI() {
	ImGui_ImplDX10_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImPlot::DestroyContext();
	ImGui::DestroyContext();

	CleanupDeviceD3D();
	::DestroyWindow(hwnd);
	::UnregisterClass(wc.lpszClassName, wc.hInstance);
}

bool GUI::beginFrame() {
	if (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
	{
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
		return false;
	}

	ImGui_ImplDX10_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	return true;
}

void GUI::endFrame() {
	//Watermark
	//ImGui::GetForegroundDrawList()->AddText(ImVec2(25,25), IM_COL32(255, 255, 255, 100), "Made by Filip Nejedly at UTB 2022", NULL);

	// Rendering
	ImGui::Render();
	g_pd3dDevice->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
	g_pd3dDevice->ClearRenderTargetView(g_mainRenderTargetView, (float*)&clear_color);
	ImGui_ImplDX10_RenderDrawData(ImGui::GetDrawData());

	//g_pSwapChain->Present(1, 0); // Present with vsync
	g_pSwapChain->Present(0, 0); // Present without vsync
}

MSG GUI::getMsg() {
	return msg;
}
