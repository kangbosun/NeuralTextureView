#include "ImGuiHandler.h"

void ImGuiHandler::Initialize(HWND hwnd, D3D12GraphicsDevice& device)
{
    if (bInitialized)
    {
        return;
    }

    if (hwnd == NULL || !device.GetDevice())
    {
        return;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    ImGui_ImplWin32_Init(hwnd);
    // ImGui DX12 init
    ImGui_ImplDX12_InitInfo initInfo = {};
    initInfo.Device = device.GetDevice();
    initInfo.CommandQueue = device.GetCommandQueue();
    initInfo.NumFramesInFlight = 2;
    initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    initInfo.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    initInfo.SrvDescriptorHeap = device.GetSrvHeap();
    initInfo.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle)
    {
        heapAllocator.Alloc(out_cpu_desc_handle, out_gpu_desc_handle);
    };
    initInfo.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_desc_handle)
    {
        heapAllocator.Free(cpu_desc_handle, gpu_desc_handle);
    };

    ImGui_ImplDX12_Init(&initInfo);

	bInitialized = true;
}

void ImGuiHandler::NewFrame()
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void ImGuiHandler::Render(D3D12GraphicsDevice& device)
{
    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), device.GetCommandList());
}

void ImGuiHandler::Destroy()
{
	ImGui_ImplDX12_InvalidateDeviceObjects();
	ImGui_ImplDX12_CreateDeviceObjects();
}

void ImGuiHandler::Shutdown()
{
	if (!bInitialized)
	{
		return;
	}

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    bInitialized = false;
}

void ImGuiHandler::InvalidateDeviceObjects()
{
	ImGui_ImplDX12_InvalidateDeviceObjects();
}
