
#include "Window.h"



void Window::Init(int width, int height, WNDPROC WndProc)
{
	this->width = width;
	this->height = height;
	//register window class
	WNDCLASS wc = {};
	wc.lpfnWndProc = WndProc;
	wc.hInstance = GetModuleHandle(0);
	wc.lpszClassName = L"Window";
	//cursor
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	RegisterClass(&wc);
	//create window
	hwnd = CreateWindow(wc.lpszClassName, L"Window", WS_OVERLAPPEDWINDOW, 0, 0, width, height, 0, 0, wc.hInstance, 0);
	//show window
	ShowWindow(hwnd, SW_SHOW);
}

void Window::Resize(int width, int height)
{
	this->width = width;
	this->height = height;
}

Window::~Window()
{
	DestroyWindow(hwnd);
}

void Window::Update(float deltaTime)
{
	MSG msg = {};
	while (PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

