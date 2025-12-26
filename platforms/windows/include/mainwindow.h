#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <memory>
#include <string>

namespace cutie {

class CocoEmulator;

namespace win32 {

class Win32Renderer;
class Win32Audio;
class Win32Input;

// Main application window for CutieCoCo Windows native UI
class MainWindow {
public:
    MainWindow();
    ~MainWindow();

    // Window creation and lifecycle
    bool create(HINSTANCE hInstance, int nCmdShow);
    int run();  // Main message loop

    // Window handle accessor
    HWND handle() const { return m_hwnd; }

private:
    // Window procedure
    static LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT handleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    // Message handlers
    void onCreate();
    void onDestroy();
    void onPaint();
    void onSize(int width, int height);
    void onKeyDown(WPARAM vkCode, LPARAM flags);
    void onKeyUp(WPARAM vkCode, LPARAM flags);
    void onChar(wchar_t ch);
    void onKillFocus();
    void onTimer();

    // Menu handlers
    void onFileOpen();
    void onFileReset();
    void onFileExit();
    void onHelpAbout();

    // Emulation control
    void initEmulator();
    void runFrame();

    // Window class registration
    static bool registerWindowClass(HINSTANCE hInstance);
    static const wchar_t* const CLASS_NAME;

private:
    HWND m_hwnd = nullptr;
    HINSTANCE m_hInstance = nullptr;

    // Emulator components
    std::unique_ptr<CocoEmulator> m_emulator;
    std::unique_ptr<Win32Renderer> m_renderer;
    std::unique_ptr<Win32Audio> m_audio;
    std::unique_ptr<Win32Input> m_input;

    // Timing
    UINT_PTR m_timerId = 0;
    static constexpr UINT TIMER_ID = 1;
    static constexpr UINT FRAME_INTERVAL_MS = 16;  // ~60 Hz

    // Window state
    int m_clientWidth = 640;
    int m_clientHeight = 480;
    bool m_isRunning = false;
};

}  // namespace win32
}  // namespace cutie
