#pragma once
#include "Graphics.h"
#include "imgui\imgui.h"
#include "imgui\imgui_impl_win32.h"
#include "imgui\imgui_impl_dx12.h"

class ImGuiHandler
{
public:
    void Initialize(HWND hwnd, D3D12GraphicsDevice& device);
    void NewFrame();
    void Render(D3D12GraphicsDevice& device);
	void Destroy();
    void Shutdown();
	bool IsInitialized() const { return bInitialized; }
	void InvalidateDeviceObjects();

private:
    bool vsync = false;

    //state
	bool bInitialized = false;
};

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);