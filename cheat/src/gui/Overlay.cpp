#include "gui/Overlay.h"

#include <d3d11.h>
#include <dxgi.h>
#include <windows.h>

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include "core/Hook.h"
#include "core/InputManager.h"
#include "core/ModuleManager.h"
#include "gui/ClickGui.h"

// imgui_impl_win32.h deliberately wraps this declaration in "#if 0" (to avoid forcing
// <windows.h> types on every imgui user) and tells callers to paste it themselves.
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam,
                                                               LPARAM lParam);

namespace gui {

namespace {

using PresentFn = HRESULT(__stdcall*)(IDXGISwapChain*, UINT, UINT);
using ResizeBuffersFn = HRESULT(__stdcall*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT,
                                             UINT);

PresentFn originalPresent = nullptr;
ResizeBuffersFn originalResizeBuffers = nullptr;
WNDPROC originalWndProc = nullptr;

ID3D11Device* device = nullptr;
ID3D11DeviceContext* context = nullptr;
ID3D11RenderTargetView* renderTargetView = nullptr;
HWND gameWindow = nullptr;
bool imguiInitialized = false;

void CreateRenderTarget(IDXGISwapChain* swapChain) {
    ID3D11Texture2D* backBuffer = nullptr;
    swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (!backBuffer) {
        return;
    }
    device->CreateRenderTargetView(backBuffer, nullptr, &renderTargetView);
    backBuffer->Release();
}

void ReleaseRenderTarget() {
    if (renderTargetView) {
        renderTargetView->Release();
        renderTargetView = nullptr;
    }
}

LRESULT __stdcall DetourWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ClickGui::visible) {
        ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);
        ImGuiIO& io = ImGui::GetIO();
        // Only swallow mouse input the GUI actually wants (so clicking a button doesn't
        // also register as a left-click in the world). Keyboard is deliberately never
        // blocked here: the offset finder needs WASD/Space to keep reaching the game
        // while its window has focus, since that's how you move to narrow candidates.
        // ImGui text fields still work fine either way -- they already received the key
        // via the WndProcHandler call above, independent of whether we forward it on.
        bool blockMouse = io.WantCaptureMouse && (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST);
        if (blockMouse) {
            return TRUE;
        }
    }
    return CallWindowProcW(originalWndProc, hwnd, msg, wParam, lParam);
}

void EnsureInit(IDXGISwapChain* swapChain) {
    if (imguiInitialized) {
        return;
    }

    if (FAILED(swapChain->GetDevice(IID_PPV_ARGS(&device)))) {
        return;
    }
    device->GetImmediateContext(&context);

    DXGI_SWAP_CHAIN_DESC desc{};
    swapChain->GetDesc(&desc);
    gameWindow = desc.OutputWindow;

    CreateRenderTarget(swapChain);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(gameWindow);
    ImGui_ImplDX11_Init(device, context);

    originalWndProc = reinterpret_cast<WNDPROC>(
        SetWindowLongPtrW(gameWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(DetourWndProc)));

    imguiInitialized = true;
}

HRESULT __stdcall DetourPresent(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags) {
    EnsureInit(swapChain);

    if (imguiInitialized) {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        core::InputManager::PollAndDispatch();
        core::ModuleManager::Instance().Tick();
        ClickGui::Render();

        ImGui::Render();
        context->OMSetRenderTargets(1, &renderTargetView, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    return originalPresent(swapChain, syncInterval, flags);
}

HRESULT __stdcall DetourResizeBuffers(IDXGISwapChain* swapChain, UINT bufferCount, UINT width,
                                      UINT height, DXGI_FORMAT format, UINT flags) {
    ReleaseRenderTarget();
    HRESULT result =
        originalResizeBuffers(swapChain, bufferCount, width, height, format, flags);
    if (imguiInitialized) {
        CreateRenderTarget(swapChain);
    }
    return result;
}

// Creates a throwaway device + swapchain purely to read Present/ResizeBuffers out of the
// DXGI vtable, then tears it down. This mirrors how the game's own swapchain is laid out
// without needing any offset into Minecraft.Windows.exe itself.
bool GetSwapChainVTable(void** presentSlot, void** resizeBuffersSlot) {
    WNDCLASSEXW wc{sizeof(WNDCLASSEXW)};
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"CheatDummyWindow";
    RegisterClassExW(&wc);

    HWND dummyWindow = CreateWindowExW(0, wc.lpszClassName, L"", WS_OVERLAPPEDWINDOW, 0, 0, 1,
                                        1, nullptr, nullptr, wc.hInstance, nullptr);

    DXGI_SWAP_CHAIN_DESC desc{};
    desc.BufferCount = 1;
    desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.OutputWindow = dummyWindow;
    desc.SampleDesc.Count = 1;
    desc.Windowed = TRUE;
    desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL featureLevel;
    IDXGISwapChain* dummySwapChain = nullptr;
    ID3D11Device* dummyDevice = nullptr;
    ID3D11DeviceContext* dummyContext = nullptr;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &desc,
        &dummySwapChain, &dummyDevice, &featureLevel, &dummyContext);

    bool ok = false;
    if (SUCCEEDED(hr)) {
        void** vtable = *reinterpret_cast<void***>(dummySwapChain);
        *presentSlot = vtable[8];          // IDXGISwapChain::Present
        *resizeBuffersSlot = vtable[13];   // IDXGISwapChain::ResizeBuffers
        ok = true;

        dummyContext->Release();
        dummyDevice->Release();
        dummySwapChain->Release();
    }

    DestroyWindow(dummyWindow);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return ok;
}

}  // namespace

bool Overlay::Install() {
    void* presentAddr = nullptr;
    void* resizeBuffersAddr = nullptr;
    if (!GetSwapChainVTable(&presentAddr, &resizeBuffersAddr)) {
        return false;
    }

    if (!core::Hook::Attach(presentAddr, &DetourPresent,
                             reinterpret_cast<void**>(&originalPresent))) {
        return false;
    }

    if (!core::Hook::Attach(resizeBuffersAddr, &DetourResizeBuffers,
                             reinterpret_cast<void**>(&originalResizeBuffers))) {
        return false;
    }

    return true;
}

void Overlay::Uninstall() {
    if (originalWndProc && gameWindow) {
        SetWindowLongPtrW(gameWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(originalWndProc));
    }
    if (imguiInitialized) {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }
    ReleaseRenderTarget();
}

HWND GetGameWindow() { return gameWindow; }

}  // namespace gui
