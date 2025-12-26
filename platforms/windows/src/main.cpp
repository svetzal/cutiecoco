// CutieCoCo - Windows Native Application Entry Point

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "mainwindow.h"

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE /*hPrevInstance*/,
    _In_ LPWSTR /*lpCmdLine*/,
    _In_ int nCmdShow)
{
    // Initialize COM for potential future use (DirectX, shell dialogs, etc.)
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        MessageBoxW(nullptr, L"Failed to initialize COM", L"CutieCoCo", MB_ICONERROR);
        return 1;
    }

    int result = 0;

    {
        cutie::win32::MainWindow mainWindow;

        if (!mainWindow.create(hInstance, nCmdShow)) {
            MessageBoxW(nullptr, L"Failed to create main window", L"CutieCoCo", MB_ICONERROR);
            result = 1;
        } else {
            result = mainWindow.run();
        }
    }

    CoUninitialize();
    return result;
}
