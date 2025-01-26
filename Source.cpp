
#include <iostream>
#include <windowsx.h>
#include <filesystem>
#include <windows.h>
#include "psapi.h"

#include "AppState.h"
#include "Texture2D.h"
#include <pix3.h>
#include "NeuralModel.h"
#include "Material.h"
#include "Shader.h"

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
		return 0;
	//messaage loop
	switch (msg)
	{
	case WM_DESTROY:
		gAppState.bRquestedExit = true;
		PostQuitMessage(0);
		break;
		//window resize
	case WM_SIZE:
	{
		//new size
		int width = LOWORD(lparam);
		int height = HIWORD(lparam);

		//resize window
		gAppState.window.Resize(width, height);

		//invalidate imgui
		gAppState.imguiHandler.InvalidateDeviceObjects();

		//resize d3d12 device
		if (gAppState.device.GetDevice() != nullptr && wparam != SIZE_MINIMIZED)
		{
			gAppState.device.Resize(width, height);
		}
		break;
	}
		//default
	default:
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}

	return 0;	
}

// FrameTime Counter class
class FrameTimer
{
public:
	FrameTimer()
	{
		QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSecond);
		secondsPerCount = 1.0f / countsPerSecond;
		QueryPerformanceCounter((LARGE_INTEGER*)&prevTime);
	}

	float GetDeltaTime()
	{
		__int64 currentTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&currentTime);
		float deltaTime = (currentTime - prevTime) * secondsPerCount;
		prevTime = currentTime;
		return deltaTime;
	}

	//update state
	void Update(float deltaTime)
	{
		frameCounter++;
		elapsedTime += deltaTime;
		if (elapsedTime >= 1.0f)
		{
			fps = (float)frameCounter;
			frameCounter = 0;
			elapsedTime = 0.0f;
		}
	}

	//get fps
	float GetFPS() const
	{
		return fps;
	}

	//get frame time
	float GetFrameTime() const
	{
		return 1.0f / fps;
	}
private:
	__int64 countsPerSecond;
	float secondsPerCount;
	__int64 prevTime;
	int frameCounter = 0;
	float fps = 0.0f;
	float elapsedTime = 0.0f;
};

//struct for Performance stats
struct PerformanceStats
{
	float fps = 0.0f;
	float frameTime = 0.0f;

	//memory
	float memoryUsage = 0.0f;
	float memoryBudget = 0.0f;
};

//global instance
PerformanceStats gPerformanceStats;

//extract function draw texture info to imgui
void DrawTextureInfo(const Texture2D& texture)
{
	//get name from texture, wstring to string
	std::string name;
	name.assign(texture.name.begin(), texture.name.end());
	//draw texture name
	ImGui::Text(name.c_str());

	ImGui::Image(texture.gpuHandle.ptr, ImVec2(128, 128));
	//width, height, format in a line
	auto Desc = texture.GetDesc();
	ImGui::Text("%dx%d(%ls)", Desc.Width, Desc.Height, Desc.Format.c_str());

	//byte size in MB
	float uncompressedMB = (float)texture.uncompressedByteSize / 1024.0f / 1024.0f;
	float compressedMB = (float)texture.compressedByteSize / 1024.0f / 1024.0f;
	ImGui::Text("SizeInMem: %.2fMB", uncompressedMB);

}

void ImGuiTextureCheckBox(bool& bEnable, const std::string& name, std::shared_ptr<Texture2D>& Texture,  
	std::shared_ptr<Texture2D>& enabledTexture, std::shared_ptr<Texture2D>& disabledTexture = Texture2D::WhiteTexture)
{
	if (ImGui::Checkbox(name.c_str(), &bEnable))
	{
		if (bEnable)
		{
			Texture = enabledTexture;
		}
		else
		{

			Texture = disabledTexture;
		}
	}
}

void main(int argc, char** argv)
{
	for (int i = 0; i < argc; i++)
	{
		gAppState.arguments.push_back(argv[i]);

		std::cout << "Argument " << i << ": " << argv[i] << std::endl;
	}

	//create window sized 800x600
	gAppState.window.Init(1920, 1080, WndProc);
	Window& window = gAppState.window;

	D3D12GraphicsDevice& device = gAppState.device;

	//create d3d12 graphics device
	device.Initialize(window.GetHandle(), window.GetWidth(), window.GetHeight());

	//frame timer
	FrameTimer frameTimer;

	//get total physical memory budget
	MEMORYSTATUSEX statex;
	statex.dwLength = sizeof(statex);
	GlobalMemoryStatusEx(&statex);
	gPerformanceStats.memoryBudget = (float)statex.ullTotalPhys / 1024.0f / 1024.0f;

	auto model = NeuralModel::LoadModel("textures/NeuralCompressed/v23/decodermodel.json");
	
	gAppState.imguiHandler.Initialize(window.GetHandle(), device);
	ImGuiHandler& ImGuiHandler = gAppState.imguiHandler;
	
	std::unordered_map<std::string, MaterialPtr> materialMap;

	auto Texture4K = Texture2D::CreateFromFile(device, L"textures/PavingStones131_4K-Color.png");
	auto NormalTexture = Texture2D::CreateFromFile(device, L"textures/PavingStones131_4K-NormalDX.png");
	auto AOTexture = Texture2D::CreateFromFile(device, L"textures/PavingStones131_4K-AO.png");
	auto RoughnessTexture = Texture2D::CreateFromFile(device, L"textures/PavingStones131_4K-Roughness.png");

	MaterialPtr material_4K_png = std::make_shared<Material>();
	material_4K_png->textures.push_back(Texture4K);
	material_4K_png->textures.push_back(NormalTexture);
	material_4K_png->textures.push_back(AOTexture);
	material_4K_png->textures.push_back(RoughnessTexture);

	material_4K_png->vertexShader = ShaderMap::Get().GetShader<VertexShader>(device, "FullScreenRectVS");
	material_4K_png->pixelShader = ShaderMap::Get().GetShader<PixelShader>(device, "FullScreenRectPS");

	materialMap["4K_PNG"] = material_4K_png;

	auto Texture4KDDS = Texture2D::CreateFromDDS(device, L"textures/4K_DDS/PavingStones131_4K-Color.dds");
	auto Normal4KDDS = Texture2D::CreateFromDDS(device, L"textures/4K_DDS/PavingStones131_4K-NormalDX.dds");
	auto AO4KDDS = Texture2D::CreateFromDDS(device, L"textures/4K_DDS/PavingStones131_4K-AO.dds");
	auto Roughness4KDDS = Texture2D::CreateFromDDS(device, L"textures/4K_DDS/PavingStones131_4K-Roughness.dds");

	MaterialPtr material_4K_dds = std::make_shared<Material>();
	material_4K_dds->textures.push_back(Texture4KDDS);
	material_4K_dds->textures.push_back(Normal4KDDS);
	material_4K_dds->textures.push_back(AO4KDDS);
	material_4K_dds->textures.push_back(Roughness4KDDS);

	material_4K_dds->vertexShader = ShaderMap::Get().GetShader<VertexShader>(device, "FullScreenRectVS");
	material_4K_dds->pixelShader = ShaderMap::Get().GetShader<PixelShader>(device, "FullScreenRectPS");

	materialMap["4K_DDS"] = material_4K_dds;

	auto Texture1KDDS = Texture2D::CreateFromDDS(device, L"textures/1K_DDS/PavingStones131_1K-Color.dds");
	auto Normal1KDDS = Texture2D::CreateFromDDS(device, L"textures/1K_DDS/PavingStones131_1K-NormalDX.dds");
	auto AO1KDDS = Texture2D::CreateFromDDS(device, L"textures/1K_DDS/PavingStones131_1K-AO.dds");
	auto Roughness1KDDS = Texture2D::CreateFromDDS(device, L"textures/1K_DDS/PavingStones131_1K-Roughness.dds");

	MaterialPtr material_1K_dds = std::make_shared<Material>();
	material_1K_dds->textures.push_back(Texture1KDDS);
	material_1K_dds->textures.push_back(Normal1KDDS);
	material_1K_dds->textures.push_back(AO1KDDS);
	material_1K_dds->textures.push_back(Roughness1KDDS);

	material_1K_dds->vertexShader = ShaderMap::Get().GetShader<VertexShader>(device, "FullScreenRectVS");
	material_1K_dds->pixelShader = ShaderMap::Get().GetShader<PixelShader>(device, "FullScreenRectPS");

	materialMap["1K_DDS"] = material_1K_dds;

	auto featuregrid0 = Texture2D::CreateFromDDS(device, L"textures/NeuralCompressed/v23/compressed0.dds");
	auto featuregrid1 = Texture2D::CreateFromDDS(device, L"textures/NeuralCompressed/v23/compressed1.dds");
	auto featuregrid2 = Texture2D::CreateFromDDS(device, L"textures/NeuralCompressed/v23/compressed2.dds");
	auto featuregrid3 = Texture2D::CreateFromDDS(device, L"textures/NeuralCompressed/v23/compressed3.dds");
	
	auto material_1k_neural = std::make_shared<NeuralTextureMaterial>();
	material_1k_neural->textures.push_back(featuregrid0);
	material_1k_neural->textures.push_back(featuregrid1);
	material_1k_neural->textures.push_back(featuregrid2);
	material_1k_neural->textures.push_back(featuregrid3);

	material_1k_neural->vertexShader = ShaderMap::Get().GetShader<VertexShader>(device, "FullScreenRectVS");
	material_1k_neural->pixelShader = ShaderMap::Get().GetShader<NeuralPixelShader>(device, "NeuralFullScreenRectPS");

	material_1k_neural->model = model;

	materialMap["1K_Neural"] = material_1k_neural;


	// light weight neural texture material 1k, 32 nodes
	auto featuregrid0_light_1k = Texture2D::CreateFromDDS(device, L"textures/NeuralCompressed/1024_32/compressed0.dds");
	auto featuregrid1_light_1k = Texture2D::CreateFromDDS(device, L"textures/NeuralCompressed/1024_32/compressed1.dds");
	auto featuregrid2_light_1k = Texture2D::CreateFromDDS(device, L"textures/NeuralCompressed/1024_32/compressed2.dds");
	auto featuregrid3_light_1k = Texture2D::CreateFromDDS(device, L"textures/NeuralCompressed/1024_32/compressed3.dds");

	auto material_light_neural_1k = std::make_shared<NeuralTextureMaterial>();
	material_light_neural_1k->textures.push_back(featuregrid0_light_1k);
	material_light_neural_1k->textures.push_back(featuregrid1_light_1k);
	material_light_neural_1k->textures.push_back(featuregrid2_light_1k);
	material_light_neural_1k->textures.push_back(featuregrid3_light_1k);

	material_light_neural_1k->vertexShader = ShaderMap::Get().GetShader<VertexShader>(device, "FullScreenRectVS");
	material_light_neural_1k->pixelShader = ShaderMap::Get().GetShader<NeuralPixelShaderLight<32>>(device, "Light32NeuralFullScreenRectPS");

	material_light_neural_1k->model = NeuralModel::LoadModel("textures/NeuralCompressed/1024_32/decodermodel.json");

	materialMap["1K_Neural_Light_32"] = material_light_neural_1k;

	// light weight neural texture material 2k, 32 nodes
	auto featuregrid0_light_2k = Texture2D::CreateFromDDS(device, L"textures/NeuralCompressed/2048_32/compressed0.dds");
	auto featuregrid1_light_2k = Texture2D::CreateFromDDS(device, L"textures/NeuralCompressed/2048_32/compressed1.dds");
	auto featuregrid2_light_2k = Texture2D::CreateFromDDS(device, L"textures/NeuralCompressed/2048_32/compressed2.dds");
	auto featuregrid3_light_2k = Texture2D::CreateFromDDS(device, L"textures/NeuralCompressed/2048_32/compressed3.dds");

	auto material_light_neural_2k = std::make_shared<NeuralTextureMaterial>();
	material_light_neural_2k->textures.push_back(featuregrid0_light_2k);
	material_light_neural_2k->textures.push_back(featuregrid1_light_2k);
	material_light_neural_2k->textures.push_back(featuregrid2_light_2k);
	material_light_neural_2k->textures.push_back(featuregrid3_light_2k);

	material_light_neural_2k->vertexShader = ShaderMap::Get().GetShader<VertexShader>(device, "FullScreenRectVS");
	material_light_neural_2k->pixelShader = ShaderMap::Get().GetShader<NeuralPixelShaderLight<32>>(device, "Light32NeuralFullScreenRectPS");

	material_light_neural_2k->model = NeuralModel::LoadModel("textures/NeuralCompressed/2048_32/decodermodel.json");

	materialMap["2K_Neural_Light_32"] = material_light_neural_2k;

	auto CurrentMaterial = material_4K_png;

	//main loop
	while (!gAppState.bRquestedExit)
	{
		//delta time
		float deltaTime = frameTimer.GetDeltaTime();

		//update frame timer
		frameTimer.Update(deltaTime);

		//fps
		float fps = frameTimer.GetFPS();
		//prevent fps set to 0
		if (fps == 0.0f)
		{
			fps = 1.0f;
		}

		gPerformanceStats.fps = fps;
		gPerformanceStats.frameTime = frameTimer.GetFrameTime();
		
		//get memory usage of this process
		PROCESS_MEMORY_COUNTERS_EX pmc;
		GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
		gPerformanceStats.memoryUsage = (float)pmc.PrivateUsage / 1024.0f / 1024.0f;
		
		window.Update(deltaTime);

		device.PreRender();

		//psudeo code to Add pass to draw fullscreen rect
		device.DrawFullScreenRect(CurrentMaterial);

		//render
		device.Render(deltaTime);


		if (ImGuiHandler.IsInitialized())
		{
			PIXScopedEvent(device.GetCommandList(), 0, L"ImGui");

			//	//imgui render
			ImGuiHandler.NewFrame();

			//window pos is right upper
			const int uiwidth = 300;
			int posX = window.GetWidth() - 300;
			int posY = 0;
			ImGui::SetNextWindowPos(ImVec2((float)posX, (float)posY));
			ImGui::SetNextWindowSize(ImVec2(uiwidth, 400), ImGuiCond_Once);
			ImGui::SetNextWindowBgAlpha(0.35f);
			
			ImGuiWindowFlags window_flags = 0;
			window_flags |= ImGuiWindowFlags_NoTitleBar;

			bool open = false;
			ImGui::Begin("Performance", &open, window_flags);
			ImGui::Text("FPS: %.2f", frameTimer.GetFPS());
			ImGui::Text("Frame Time: %.2fms", frameTimer.GetFrameTime() * 1000);
			ImGui::Separator();
			ImGui::Text("Memory Usage: %.2f/%.2fGB", gPerformanceStats.memoryUsage / 1024.0f, gPerformanceStats.memoryBudget / 1024.0f);


			static bool vsync = false;
			ImGui::Checkbox("VSYNC", &vsync);
			device.SetVsync(vsync);

			//Slider to change view scale
			ImGui::SeparatorText("View");
			ImGui::SliderFloat("View Scale", &device.rectConstantBuffer.scale, 0.2f, 1.0f);

			//Slider to change light position
			ImGui::SeparatorText("Light");
			ImGui::SliderFloat("PosX", &device.rectConstantBuffer.lightpos, -1.0f, 1.0f);

			//Slider to change light intensity
			ImGui::SliderFloat("Intensity", &device.rectConstantBuffer.intensity, 0.0f, 5.0f);

			ImGui::SeparatorText("Material");
			ImGui::SliderFloat("Metalness", &device.rectConstantBuffer.metalness, 0.0f, 1.0f);

			ImGui::End();

			//Resource Description view window
			posY = 400;
			ImGui::SetNextWindowPos(ImVec2((float)posX, (float)posY));
			ImGui::SetNextWindowSize(ImVec2(300, 500), ImGuiCond_Once);
			ImGui::SetNextWindowBgAlpha(0.35f);

			ImGui::Begin("Material Description", &open);

			static int currentItem = 0;	
			std::vector<const char*> items;
			for (auto& item : materialMap)
			{
				items.push_back(item.first.c_str());
			}

			ImGui::Combo("Material", &currentItem, items.data(), (int)items.size());

			CurrentMaterial = materialMap[items[currentItem]];
			
			//draw texture info
			for (auto& texture : CurrentMaterial->textures)
			{
				ImGui::Separator();

				DrawTextureInfo(*texture);
			}
				

			ImGui::End();


			ImGuiHandler.Render(device);
		}

		device.Present();

		device.PostRender();
	}

	// Cleanup
	ImGuiHandler.Shutdown();
	device.Cleanup();
}

