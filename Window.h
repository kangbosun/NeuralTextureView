
#pragma
#include <Windows.h>

class Window
{
public:
	//constructor
	Window() = default;

	~Window();

	//with winproc
	void Init(int width, int height, WNDPROC proc);
	void Resize(int width, int height);

	void Update(float deltaTime);
	HWND GetHandle() const { return hwnd; }
	int GetWidth() const { return width; }
	int GetHeight() const { return height; }

private:
	int width = 0;
	int height = 0;
	//window handle
	HWND hwnd = 0;
};