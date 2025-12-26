// CutieCoCo - Windows Main Window Implementation

#include "mainwindow.h"
#include "win32renderer.h"
#include "win32audio.h"
#include "win32input.h"

#include "cutie/emulator.h"

#include <commdlg.h>
#include <cstdio>

namespace cutie {
namespace win32 {

const wchar_t* const MainWindow::CLASS_NAME = L"CutieCoCoWindowClass";

// Menu IDs
enum MenuID {
    ID_FILE_OPEN = 1001,
    ID_FILE_RESET,
    ID_FILE_EXIT,
    ID_HELP_ABOUT,
};

MainWindow::MainWindow() = default;

MainWindow::~MainWindow()
{
    if (m_timerId) {
        KillTimer(m_hwnd, m_timerId);
    }
}

bool MainWindow::registerWindowClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = windowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = CLASS_NAME;
    // TODO: Load icon from resources
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

    return RegisterClassExW(&wc) != 0;
}

bool MainWindow::create(HINSTANCE hInstance, int nCmdShow)
{
    m_hInstance = hInstance;

    if (!registerWindowClass(hInstance)) {
        return false;
    }

    // Create menu
    HMENU hMenu = CreateMenu();
    HMENU hFileMenu = CreatePopupMenu();
    AppendMenuW(hFileMenu, MF_STRING, ID_FILE_OPEN, L"&Open ROM...\tCtrl+O");
    AppendMenuW(hFileMenu, MF_STRING, ID_FILE_RESET, L"&Reset\tF5");
    AppendMenuW(hFileMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hFileMenu, MF_STRING, ID_FILE_EXIT, L"E&xit\tAlt+F4");
    AppendMenuW(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hFileMenu), L"&File");

    HMENU hHelpMenu = CreatePopupMenu();
    AppendMenuW(hHelpMenu, MF_STRING, ID_HELP_ABOUT, L"&About CutieCoCo...");
    AppendMenuW(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hHelpMenu), L"&Help");

    // Calculate window size for desired client area
    RECT rect = {0, 0, m_clientWidth, m_clientHeight};
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, TRUE);
    int windowWidth = rect.right - rect.left;
    int windowHeight = rect.bottom - rect.top;

    m_hwnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"CutieCoCo - Tandy Color Computer 3 Emulator",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        windowWidth, windowHeight,
        nullptr,
        hMenu,
        hInstance,
        this  // Pass this pointer for WM_CREATE
    );

    if (!m_hwnd) {
        return false;
    }

    ShowWindow(m_hwnd, nCmdShow);
    UpdateWindow(m_hwnd);

    return true;
}

int MainWindow::run()
{
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK MainWindow::windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    MainWindow* self = nullptr;

    if (msg == WM_NCCREATE) {
        auto cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        self = static_cast<MainWindow*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->m_hwnd = hwnd;
    } else {
        self = reinterpret_cast<MainWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (self) {
        return self->handleMessage(msg, wParam, lParam);
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT MainWindow::handleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_CREATE:
            onCreate();
            return 0;

        case WM_DESTROY:
            onDestroy();
            PostQuitMessage(0);
            return 0;

        case WM_PAINT:
            onPaint();
            return 0;

        case WM_SIZE:
            onSize(LOWORD(lParam), HIWORD(lParam));
            return 0;

        case WM_KEYDOWN:
            onKeyDown(wParam, lParam);
            return 0;

        case WM_KEYUP:
            onKeyUp(wParam, lParam);
            return 0;

        case WM_CHAR:
            onChar(static_cast<wchar_t>(wParam));
            return 0;

        case WM_KILLFOCUS:
            onKillFocus();
            return 0;

        case WM_TIMER:
            if (wParam == TIMER_ID) {
                onTimer();
            }
            return 0;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_FILE_OPEN:
                    onFileOpen();
                    return 0;
                case ID_FILE_RESET:
                    onFileReset();
                    return 0;
                case ID_FILE_EXIT:
                    onFileExit();
                    return 0;
                case ID_HELP_ABOUT:
                    onHelpAbout();
                    return 0;
            }
            break;
    }

    return DefWindowProc(m_hwnd, msg, wParam, lParam);
}

void MainWindow::onCreate()
{
    // Initialize renderer
    m_renderer = std::make_unique<Win32Renderer>();
    if (!m_renderer->init(m_hwnd)) {
        MessageBoxW(m_hwnd, L"Failed to initialize renderer", L"CutieCoCo", MB_ICONWARNING);
    }

    // Initialize audio
    m_audio = std::make_unique<Win32Audio>();
    if (!m_audio->init(44100)) {
        MessageBoxW(m_hwnd, L"Failed to initialize audio", L"CutieCoCo", MB_ICONWARNING);
    }

    // Initialize input handler
    m_input = std::make_unique<Win32Input>();

    // Initialize emulator
    initEmulator();

    // Connect input handler to emulator for joystick updates
    if (m_emulator) {
        m_input->setEmulator(m_emulator.get());
    }

    // Start frame timer
    m_timerId = SetTimer(m_hwnd, TIMER_ID, FRAME_INTERVAL_MS, nullptr);
    m_isRunning = true;
}

void MainWindow::onDestroy()
{
    m_isRunning = false;

    if (m_timerId) {
        KillTimer(m_hwnd, m_timerId);
        m_timerId = 0;
    }

    if (m_input) {
        m_input->reset();
        m_input.reset();
    }

    if (m_emulator) {
        m_emulator->shutdown();
        m_emulator.reset();
    }

    if (m_audio) {
        m_audio->shutdown();
        m_audio.reset();
    }

    if (m_renderer) {
        m_renderer->shutdown();
        m_renderer.reset();
    }
}

void MainWindow::onPaint()
{
    PAINTSTRUCT ps;
    BeginPaint(m_hwnd, &ps);
    // Rendering is done in onTimer via the renderer
    EndPaint(m_hwnd, &ps);
}

void MainWindow::onSize(int width, int height)
{
    m_clientWidth = width;
    m_clientHeight = height;

    if (m_renderer) {
        m_renderer->resize(width, height);
    }
}

void MainWindow::onKeyDown(WPARAM vkCode, LPARAM flags)
{
    if (m_input) {
        m_input->handleKeyDown(vkCode, flags);
    }
}

void MainWindow::onKeyUp(WPARAM vkCode, LPARAM flags)
{
    if (m_input) {
        m_input->handleKeyUp(vkCode, flags);
    }
}

void MainWindow::onChar(wchar_t ch)
{
    if (m_input) {
        m_input->handleChar(ch);
    }
}

void MainWindow::onKillFocus()
{
    // Reset all input when window loses focus
    if (m_input) {
        m_input->reset();
    }
}

void MainWindow::onTimer()
{
    if (!m_isRunning || !m_emulator) return;

    runFrame();
}

void MainWindow::initEmulator()
{
    EmulatorConfig config;
    config.memorySize = MemorySize::Mem512K;
    config.cpuType = CpuType::MC6809;
    config.audioSampleRate = 44100;
    // TODO: Configure system ROM path

    m_emulator = CocoEmulator::create(config);
    if (m_emulator && !m_emulator->init()) {
        MessageBoxW(m_hwnd, L"Failed to initialize emulator", L"CutieCoCo", MB_ICONERROR);
        m_emulator.reset();
    }
}

void MainWindow::runFrame()
{
    if (!m_emulator) return;

    m_emulator->runFrame();

    // Get and display frame
    auto [pixels, size] = m_emulator->getFramebuffer();
    if (pixels && m_renderer) {
        m_renderer->beginFrame();
        m_renderer->present(pixels, 640, 480);  // TODO: Get actual frame dimensions
        m_renderer->endFrame();
    }

    // Get and play audio
    auto [samples, sampleCount] = m_emulator->getAudioSamples();
    if (samples && sampleCount > 0 && m_audio) {
        m_audio->submitSamples(samples, sampleCount);
    }
}

void MainWindow::onFileOpen()
{
    wchar_t filename[MAX_PATH] = {};

    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hwnd;
    ofn.lpstrFilter = L"CoCo ROMs\0*.rom;*.ccc;*.pak\0All Files\0*.*\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    ofn.lpstrTitle = L"Open Cartridge ROM";

    if (GetOpenFileNameW(&ofn)) {
        // TODO: Load the ROM file (DREAM-VCC-6u8)
        MessageBoxW(m_hwnd, filename, L"ROM Loading Not Yet Implemented", MB_ICONINFORMATION);
    }
}

void MainWindow::onFileReset()
{
    if (m_emulator) {
        m_emulator->reset();
    }
}

void MainWindow::onFileExit()
{
    DestroyWindow(m_hwnd);
}

void MainWindow::onHelpAbout()
{
    MessageBoxW(m_hwnd,
        L"CutieCoCo - Tandy Color Computer 3 Emulator\n\n"
        L"A cross-platform CoCo 3 emulator derived from VCC.\n\n"
        L"https://github.com/your-repo/cutiecoco",
        L"About CutieCoCo",
        MB_ICONINFORMATION);
}

}  // namespace win32
}  // namespace cutie
