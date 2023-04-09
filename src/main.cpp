#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>

// The min/max macros conflict with like-named member functions.
// Only use std::min and std::max defined in <algorithm>.
#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

// In order to define a function called CreateWindow, the Windows macro needs to
// be undefined.
#if defined(CreateWindow)
#undef CreateWindow
#endif

// Windows Runtime Library. Needed for Microsoft::WRL::ComPtr<> template class.
#include <wrl.h>
using namespace Microsoft::WRL;

// DirectX 12 specific headers.
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

// D3D12 extension library.
#include <d3dx12.h>

// STL Headers
#include <algorithm>
#include <cassert>
#include <chrono>

// Helper functions
#include <Helpers.h>

// The number of swap chain back buffers.
const uint8_t g_NumFrames = 3;
// Use WARP adapter
bool g_UseWarp = false;

uint32_t g_ClientWidth = 1280;
uint32_t g_ClientHeight = 720;

// Set to true once the DX12 objects have been initialized.
bool g_IsInitialized = false;

// Enable the D3D12 debug layer.
void EnableDebugLayer() {
#if defined(_DEBUG)
	ComPtr<ID3D12Debug> debugInterface;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
	debugInterface->EnableDebugLayer();
#endif
}

// Window handle.
HWND g_hWnd;
// Window rectangle (used to toggle fullscreen state).
RECT g_WindowRect;

// DirectX 12 Objects
ComPtr<ID3D12Device2> g_Device;
ComPtr<ID3D12CommandQueue> g_CommandQueue;
ComPtr<ID3D12GraphicsCommandList> g_CommandList;
ComPtr<ID3D12CommandAllocator> g_CommandAllocator;
ComPtr<IDXGISwapChain4> g_SwapChain;

// Synchronization objects
ComPtr<ID3D12Fence> g_Fence;

// Window callback function.
//LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

void ParseCommandLineArguments() {
	int argc;
	wchar_t** argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);

	for (size_t i = 0; i < argc; ++i)
	{
		if (::wcscmp(argv[i], L"-w") == 0 || ::wcscmp(argv[i], L"--width") == 0)
		{
			g_ClientWidth = ::wcstol(argv[++i], nullptr, 10);
		}
		if (::wcscmp(argv[i], L"-h") == 0 || ::wcscmp(argv[i], L"--height") == 0)
		{
			g_ClientHeight = ::wcstol(argv[++i], nullptr, 10);
		}
		if (::wcscmp(argv[i], L"-warp") == 0 || ::wcscmp(argv[i], L"--warp") == 0)
		{
			g_UseWarp = true;
		}
	}

	// Free memory allocated by CommandLineToArgvW
	::LocalFree(argv);
}

//void RegisterWindowClass(HINSTANCE hInst, const wchar_t* windowClassName) {
//	// Register a window class for creating our render window with.
//	WNDCLASSEXW windowClass = {};
//
//	windowClass.cbSize = sizeof(WNDCLASSEX);
//	windowClass.style = CS_HREDRAW | CS_VREDRAW;
//	windowClass.lpfnWndProc = &WndProc;
//	windowClass.cbClsExtra = 0;
//	windowClass.cbWndExtra = 0;
//	windowClass.hInstance = hInst;
//	windowClass.hIcon = ::LoadIcon(hInst, NULL);
//	windowClass.hCursor = ::LoadCursor(NULL, IDC_ARROW);
//	windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
//	windowClass.lpszMenuName = NULL;
//	windowClass.lpszClassName = windowClassName;
//	windowClass.hIconSm = ::LoadIcon(hInst, NULL);
//
//	static ATOM atom = ::RegisterClassExW(&windowClass);
//	assert(atom > 0);
//}

HWND CreateWindow(
	const wchar_t* windowClassName,
	HINSTANCE hInst, 
	const wchar_t* windowTitle, 
	uint32_t width, 
	uint32_t height)
{
	int screenWidth = ::GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = ::GetSystemMetrics(SM_CYSCREEN);

	RECT windowRect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
	::AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	int windowWidth = windowRect.right - windowRect.left;
	int windowHeight = windowRect.bottom - windowRect.top;

	// Center the window within the screen. Clamp to 0, 0 for the top-left corner.
	int windowX = std::max<int>(0, (screenWidth - windowWidth) / 2);
	int windowY = std::max<int>(0, (screenHeight - windowHeight) / 2);

	HWND hWnd = ::CreateWindowExW(
		NULL,
		windowClassName,
		windowTitle,
		WS_OVERLAPPEDWINDOW,
		windowX,
		windowY,
		windowWidth,
		windowHeight,
		NULL,
		NULL,
		hInst,
		nullptr
	);

	assert(hWnd && "Failed to create window");

	return hWnd;
}

ComPtr<IDXGIAdapter4> GetAdapter(bool useWarp) {
	ComPtr<IDXGIFactory4> dxgiFactory;
	UINT createFactoryFlags = 0;
#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

	ComPtr<IDXGIAdapter1> dxgiAdapter1;
	ComPtr<IDXGIAdapter4> dxgiAdapter4;

	if (useWarp) {
		ThrowIfFailed(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter1)));
		ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
	} else {
		SIZE_T maxDedicatedVideoMemory = 0;

		for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i) {
			DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
			dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

			if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
				SUCCEEDED(D3D12CreateDevice(
					dxgiAdapter1.Get(),
					D3D_FEATURE_LEVEL_11_0,
					_uuidof(ID3D12Device),
					nullptr
				)) &&
				dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory)
			{
				maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
				ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
			}
		}
	}

	return dxgiAdapter4;
}

ComPtr<ID3D12Device2> GetDevice(ComPtr<IDXGIAdapter4> adapter) {
	ComPtr<ID3D12Device2> d3d12Device2;
	ThrowIfFailed(D3D12CreateDevice(
		adapter.Get(), 
		D3D_FEATURE_LEVEL_11_0, 
		IID_PPV_ARGS(&d3d12Device2)
	));

	// Enable debug messages in debug mode.
#if defined(_DEBUG)
	ComPtr<ID3D12InfoQueue> pInfoQueue;

	if (SUCCEEDED(d3d12Device2.As(&pInfoQueue))) {
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

		// Suppress whole categories of messages
		//D3D12_MESSAGE_CATEGORY Categories[] = {};

		// Suppress messages based on their severity level
		D3D12_MESSAGE_SEVERITY Severities[] =
		{
			D3D12_MESSAGE_SEVERITY_INFO
		};

		// Suppress individual messages by their ID
		D3D12_MESSAGE_ID DenyIds[] = {
			D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
			//D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
			//D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
		};

		D3D12_INFO_QUEUE_FILTER NewFilter = {};
		//NewFilter.DenyList.NumCategories = _countof(Categories);
		//NewFilter.DenyList.pCategoryList = Categories;
		NewFilter.DenyList.NumSeverities = _countof(Severities);
		NewFilter.DenyList.pSeverityList = Severities;
		NewFilter.DenyList.NumIDs = _countof(DenyIds);
		NewFilter.DenyList.pIDList = DenyIds;

		ThrowIfFailed(pInfoQueue->PushStorageFilter(&NewFilter));
	}
#endif

	return d3d12Device2;
}

ComPtr<ID3D12Fence> CreateFence(ComPtr<ID3D12Device2> device) {
	ComPtr<ID3D12Fence> fence;

	ThrowIfFailed(device->CreateFence(
		0,
		D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&fence)
	));

	return fence;
}

ComPtr<ID3D12CommandQueue> CreateCommandQueue(
	ComPtr<ID3D12Device2> device, 
	D3D12_COMMAND_LIST_TYPE type) 
{
	ComPtr<ID3D12CommandQueue> d3d12CommandQueue;

	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type = type;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 0;

	ThrowIfFailed(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&d3d12CommandQueue)));

	return d3d12CommandQueue;
}

ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(
	ComPtr<ID3D12Device2> device,
	D3D12_COMMAND_LIST_TYPE type) 
{
	ComPtr<ID3D12CommandAllocator> commandAllocator;
	ThrowIfFailed(device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator)));

	return commandAllocator;
}

ComPtr<ID3D12GraphicsCommandList> CreateCommandList(
	ComPtr<ID3D12Device2> device,
	ComPtr<ID3D12CommandAllocator> commandAllocator,
	D3D12_COMMAND_LIST_TYPE type)
{
	ComPtr<ID3D12GraphicsCommandList> commandList;

	ThrowIfFailed(device->CreateCommandList(
		0,
		type,
		commandAllocator.Get(),
		nullptr,
		IID_PPV_ARGS(&commandList)
	));

	ThrowIfFailed(commandList->Close());

	return commandList;
}

bool CheckTearingSupport() {
	BOOL allowTearing = FALSE;

	// Rather than create the DXGI 1.5 factory interface directly, we create the
	// DXGI 1.4 interface and query for the 1.5 interface. This is to enable the 
	// graphics debugging tools which will not support the 1.5 factory interface 
	// until a future update.
	ComPtr<IDXGIFactory4> factory4;

	if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory4)))) {
		ComPtr<IDXGIFactory5> factory5;

		if (SUCCEEDED(factory4.As(&factory5))) {
			if (FAILED(factory5->CheckFeatureSupport(
				DXGI_FEATURE_PRESENT_ALLOW_TEARING,
				&allowTearing, sizeof(allowTearing)
			)))
			{
				allowTearing = FALSE;
			}
		}
	}

	return allowTearing == TRUE;
}

ComPtr<IDXGISwapChain4> CreateSwapChain(
	HWND hWnd,
	ComPtr<ID3D12CommandQueue> commandQueue,
	uint32_t width,
	uint32_t height,
	uint32_t bufferCount)
{
	ComPtr<IDXGISwapChain4> dxgiSwapChain4;
	ComPtr<IDXGIFactory4> dxgiFactory4;
	UINT createFactoryFlags = 0;
#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4)));

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = FALSE;
	swapChainDesc.SampleDesc = { 1, 0 };
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = bufferCount;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	// It is recommended to always allow tearing if tearing support is available.
	swapChainDesc.Flags = CheckTearingSupport() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

	ComPtr<IDXGISwapChain1> swapChain1;
	ThrowIfFailed(dxgiFactory4->CreateSwapChainForHwnd(
		commandQueue.Get(),
		hWnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain1
	));

	// Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen
	// will be handled manually.
	ThrowIfFailed(dxgiFactory4->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));

	ThrowIfFailed(swapChain1.As(&dxgiSwapChain4));

	return dxgiSwapChain4;
}

ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(
	ComPtr<ID3D12Device2> device,
	D3D12_DESCRIPTOR_HEAP_TYPE type,
	uint32_t numDescriptors)
{
	ComPtr<ID3D12DescriptorHeap> descriptroHeap;

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = numDescriptors;
	desc.Type = type;

	ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptroHeap)));

	return descriptroHeap;
}

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{	
	const wchar_t* windowClassName = L"SomeWindow";
	//RegisterWindowClass(hInstance, windowClassName);
	//g_hWnd = CreateWindow(windowClassName, hInstance, L"Title", g_ClientWidth, g_ClientHeight);

	EnableDebugLayer();

	ComPtr<IDXGIAdapter4> adaptor = GetAdapter(g_UseWarp);

	DXGI_ADAPTER_DESC1 adaptorDesc;
	adaptor->GetDesc1(&adaptorDesc);
	std::wstring text = adaptorDesc.Description;
	OutputDebugStringW(L"-----------\n");
	OutputDebugStringW(text.c_str());
	OutputDebugStringW(L"\n-----------\n");

	g_Device = GetDevice(adaptor);
	g_Fence = CreateFence(g_Device);
	g_CommandQueue = CreateCommandQueue(g_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	g_CommandAllocator = CreateCommandAllocator(g_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	g_CommandList = CreateCommandList(g_Device, g_CommandAllocator, D3D12_COMMAND_LIST_TYPE_DIRECT);
	//g_SwapChain = CreateSwapChain(g_hWnd, g_CommandQueue, g_ClientWidth, g_ClientHeight, g_NumFrames);

	return 0;
}