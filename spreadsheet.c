#define _WIN32_WINNT    0x0501
#define WIN32_LEAN_AND_MEAN
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <commdlg.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shell32.lib")

#include "fixnum.h"
#include "cell.h"
#include "csv.h"

HWND        TheWindow;
HWND        EditBox;

TCHAR       TheFilename[MAX_PATH];

#include "ui-control.c"
#include "ui-display.c"
#include "ui-input.c"

setup_resources(HWND hwnd) {
    init_ui_display(hwnd);
    init_ui_input(hwnd);
}

LRESULT CALLBACK
WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    PAINTSTRUCT ps;
    switch (msg) {
    case WM_PAINT:
        BeginPaint(hwnd, &ps);
        paint_table(WindowBuffer, &TheTable);
        BitBlt(ps.hdc, 0, 0, WindowWidth, WindowHeight,
            WindowBuffer, 0, 0, SRCCOPY);
        EndPaint(hwnd, &ps);
        return 0;
    case WM_SIZE: wm_size(hwnd, LOWORD(lparam), HIWORD(lparam)); return 0;
    /* Focus returning from another app should continue edit */
    case WM_SETFOCUS:  if (is_editing()) SetFocus(EditBox); return 0;
    case WM_CHAR: if (wm_char(hwnd, wparam)) return 0; break;
    case WM_KEYDOWN: if (wm_keydown(hwnd, wparam)) return 0; break;
    case WM_LBUTTONDOWN: wm_lbuttondown(hwnd, LOWORD(lparam), HIWORD(lparam)); return 0;
    case WM_LBUTTONDBLCLK: wm_lbuttondblclk(hwnd, LOWORD(lparam), HIWORD(lparam)); return 0;
    case WM_ERASEBKGND: return 1;
    case WM_DROPFILES: wm_dropfiles(hwnd, (HDROP)wparam); return 0;
    case WM_CREATE: setup_resources(hwnd); return 0;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProc(hwnd,msg,wparam,lparam);
}

int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmd, int show) {
    int         argc;
    TCHAR       **argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    MSG         msg;
    WNDCLASS    wc = { CS_DBLCLKS|CS_HREDRAW|CS_VREDRAW,
        WndProc, 0, 0, 0, LoadIcon(0, IDI_APPLICATION),
        LoadCursor(0, IDC_ARROW), (HBRUSH)(COLOR_WINDOW+1), 0,
        TEXT("Window")};
    RegisterClass(&wc);
    TheWindow = CreateWindowEx(WS_EX_LAYERED | WS_EX_ACCEPTFILES,
        TEXT("Window"), TEXT(""),
        WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL,
        CW_USEDEFAULT,CW_USEDEFAULT,
        CW_USEDEFAULT,CW_USEDEFAULT,
        0, 0, GetModuleHandle(0), 0);
    
    /* Open command line file */
    if (argv[1] && !open_csv(argv[1]))
        MessageBox(TheWindow, L"Could not open the file", L"Error", MB_OK);
    else if (argv[1])
        wcscpy(TheFilename, argv[1]);
    
    while (GetMessage(&msg,0,0,0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
main() { return WinMain(0,0,0,0); }
