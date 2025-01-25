#pragma once

#include <vector>
#include <string>
#include "Window.h"
#include "Graphics.h"
#include "ImguiHandler.h"

//Global Application state struct
struct AppState
{
	//window
	Window window;
	//d3d12 device
	D3D12GraphicsDevice device;
	//imgui handler
	ImGuiHandler imguiHandler;

	bool bRquestedExit = false;

	//back buffer size
	int backBufferWidth = 0;
	int backBufferHeight = 0;

	std::vector<std::string> arguments;
};

//global app state
extern AppState gAppState;