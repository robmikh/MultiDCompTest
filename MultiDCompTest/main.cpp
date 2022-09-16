#include "pch.h"
#include "MainWindow.h"
#include <dcomp.h>

namespace winrt
{
    using namespace Windows::Foundation;
    using namespace Windows::Foundation::Numerics;
    using namespace Windows::UI;
    using namespace Windows::UI::Composition;
}

namespace util
{
    using namespace robmikh::common::uwp;
    using namespace robmikh::common::desktop;
}

int __stdcall WinMain(HINSTANCE, HINSTANCE, PSTR, int)
{
    // Initialize COM
    winrt::init_apartment();
    
    // Register our window classes
    MainWindow::RegisterWindowClass();

    // Create the DispatcherQueue that the compositor needs to run
    auto controller = util::CreateDispatcherQueueControllerForCurrentThread();

    // Create our window
    auto window = MainWindow(L"MultiDCompTest", 800, 600);

    // Initialize D3D
    auto d3dDevice = util::CreateD3DDevice();
    winrt::com_ptr<ID3D11DeviceContext> d3dContext;
    d3dDevice->GetImmediateContext(d3dContext.put());
    auto dxgiDevice = d3dDevice.as<IDXGIDevice>();

    // Create our DComp visual tree
    winrt::com_ptr<IDCompositionDevice> dcompDevice;
    winrt::check_hresult(DCompositionCreateDevice(
        dxgiDevice.get(), 
        winrt::guid_of<IDCompositionDevice>(), 
        dcompDevice.put_void()));
    winrt::com_ptr<IDCompositionTarget> dcompTarget;
    winrt::check_hresult(dcompDevice->CreateTargetForHwnd(
        window.m_window, 
        false, 
        dcompTarget.put()));
    winrt::com_ptr<IDCompositionVisual> dcompVisual;
    winrt::check_hresult(dcompDevice->CreateVisual(dcompVisual.put()));
    winrt::check_hresult(dcompTarget->SetRoot(dcompVisual.get()));

    // Make a swap chain we'll use on our dcomp visual
    auto swapChain = util::CreateDXGISwapChain(d3dDevice, 800, 600, DXGI_FORMAT_B8G8R8A8_UNORM, 3);
    winrt::com_ptr<ID3D11Texture2D> backBuffer;
    winrt::check_hresult(swapChain->GetBuffer(0, winrt::guid_of<ID3D11Texture2D>(), backBuffer.put_void()));
    // Clear to red and present
    winrt::com_ptr<ID3D11RenderTargetView> renderTargetView;
    winrt::check_hresult(d3dDevice->CreateRenderTargetView(backBuffer.get(), nullptr, renderTargetView.put()));
    float color[4] = { 1.0f, 0.0f, 0.0f, 1.0f }; // RGBA
    d3dContext->ClearRenderTargetView(renderTargetView.get(), color);
    DXGI_PRESENT_PARAMETERS presentParameters = {};
    winrt::check_hresult(swapChain->Present1(1, 0, &presentParameters));

    // Attach our swap chain and commit
    winrt::check_hresult(dcompVisual->SetContent(swapChain.get()));
    winrt::check_hresult(dcompDevice->Commit());
    
    // Create our WinRT visual tree
    auto compositor = winrt::Compositor();
    auto target = window.CreateWindowTarget(compositor);
    auto root = compositor.CreateContainerVisual();
    root.RelativeSizeAdjustment({ 1.0f, 1.0f });
    target.Root(root);
    auto visual = compositor.CreateSpriteVisual();
    visual.Size({ 100.0f, 100.0f });
    visual.Offset({ 50.0f, 50.0f, 0.0f });
    visual.Brush(compositor.CreateColorBrush(winrt::Colors::Blue()));
    root.Children().InsertAtTop(visual);

    // Message pump
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return static_cast<int>(msg.wParam);
}
