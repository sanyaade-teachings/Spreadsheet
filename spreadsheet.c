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

#define FIND_TEXT_MAX       8192
#define MAX_ROWS_FOR_FIT    500
#define MAX_COLS_FOR_FIT    500
#define MIN_FIT_WIDTH       20
#define MAX_FIT_WIDTH       300

#include "fixnum.h"
#include "cell.h"
#include "csv.h"

typedef struct {
    Table   *table;
    HWND    window;
    HWND    edit;
    HDC     bitmap;
    
    BOOL        is_selecting;
    
    unsigned    cur_row, cur_col; 
    unsigned    anchor_row, anchor_col;
    
    /* UI Input Elements */
    unsigned    mouse_mode;
    POINT       mouse_scroll_anchor;
    unsigned    resizing_col;
    
    /* UI Display Elements */
    unsigned    col_pos[65536];
    unsigned    width, height;
    unsigned    visible_rows, visible_cols;
    unsigned    first_row, first_col;
    
    char    find_text[FIND_TEXT_MAX];
    TCHAR   filename[MAX_PATH];
    OPENFILENAME open_dialog;
    FINDREPLACEA find_dialog;
} TableUI;

ATOM        StructAtom;
unsigned    WM_FIND;

TableUI*
get_tui(HWND hwnd) {
    return (void*) GetProp(hwnd, (LPCTSTR)StructAtom);
}

#include "ui-control.c"
#include "action.c"
#include "ui-display.c"
#include "ui-input.c"

setup_resources(HWND hwnd) {
    TableUI *tui = calloc(1, sizeof *tui);
    tui->table = calloc(1, sizeof *tui->table);
    tui->window = hwnd;
    
    SetProp(hwnd, (LPCTSTR)StructAtom, tui);
    init_ui_display(tui);
    init_ui_input(tui);
}

shutdown_resource(HWND hwnd) {
    free(get_tui(hwnd)->table);
    free(get_tui(hwnd));
    RemoveProp(hwnd, (LPCTSTR)StructAtom);
}

LRESULT CALLBACK
WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    PAINTSTRUCT ps;
    TableUI     *tui = get_tui(hwnd);
    
    switch (msg) {
    case WM_PAINT:
        BeginPaint(hwnd, &ps);
        paint_table(tui);
        BitBlt(ps.hdc,
            ps.rcPaint.left,
            ps.rcPaint.top,
            ps.rcPaint.right - ps.rcPaint.left,
            ps.rcPaint.bottom - ps.rcPaint.top,
            tui->bitmap,
            ps.rcPaint.left,
            ps.rcPaint.top,
            SRCCOPY);
        EndPaint(hwnd, &ps);
        return 0;
    case WM_SIZE: wm_size(tui, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)); return 0;
    /* Focus returning from another app should continue edit */
    case WM_SETFOCUS:  wm_setfocus(tui); return 0;
    case WM_CHAR: if (wm_char(tui, wparam)) return 0; else break;
    case WM_KEYDOWN: if (wm_keydown(tui, wparam)) return 0; else break;
    case WM_LBUTTONDOWN: wm_lbuttondown(tui, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)); return 0;
    case WM_LBUTTONUP: wm_lbuttonup(tui, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)); return 0;
    case WM_MOUSEMOVE: wm_mousemove(tui, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)); return 0;
    case WM_LBUTTONDBLCLK: wm_lbuttondblclk(tui, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)); return 0;
    case WM_MOUSEWHEEL: wm_mousewheel(tui, GET_WHEEL_DELTA_WPARAM(wparam)); return 0;
    case WM_MBUTTONDOWN: wm_mbuttondown(tui, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)); return 0;
    case WM_MBUTTONUP: wm_mbuttonup(tui, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)); return 0;
    case WM_ERASEBKGND: return 1;
    case WM_DROPFILES: wm_dropfiles(tui, (HDROP)wparam); return 0;
    case WM_CREATE: setup_resources(hwnd); return 0;
    case WM_DESTROY: shutdown_resource(hwnd); PostQuitMessage(0); return 0;
    }
    if (msg == WM_FIND) { wm_find(tui, (FINDREPLACE*)lparam); return 0; }
    return DefWindowProc(hwnd,msg,wparam,lparam);
}

int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmd, int show) {
    int         argc;
    TCHAR       **argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    MSG         msg;
    HWND        hwnd;
    WNDCLASS    wc = { CS_DBLCLKS|CS_HREDRAW|CS_VREDRAW,
        WndProc, 0, 0, 0, LoadIcon(0, IDI_APPLICATION),
        LoadCursor(0, IDC_ARROW), (HBRUSH)(COLOR_WINDOW+1), 0,
        TEXT("Window")};
    RegisterClass(&wc);
    
    StructAtom = AddAtom(L"TableUIStruct");
    WM_FIND = RegisterWindowMessage(FINDMSGSTRING);
    
    hwnd = CreateWindowEx(WS_EX_LAYERED | WS_EX_ACCEPTFILES,
        TEXT("Window"), TEXT(""),
        WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL,
        CW_USEDEFAULT,CW_USEDEFAULT,
        CW_USEDEFAULT,CW_USEDEFAULT,
        0, 0, GetModuleHandle(0), 0);
    
    /* Open command line file */
    if (argv[1]) {
        wcscpy(get_tui(hwnd)->filename, argv[1]);
        command(get_tui(hwnd), CmdOpenFile);
    }
    
    while (GetMessage(&msg,0,0,0)) {
        if (dlg && IsDialogMessage(dlg, &msg))
			continue;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
//main() { return WinMain(0,0,0,0); }
