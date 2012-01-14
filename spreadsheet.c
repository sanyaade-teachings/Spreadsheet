#define _WIN32_WINNT    0x0501
#define WIN32_LEAN_AND_MEAN
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#pragma comment(lib, "shell32.lib")

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fixnum.h"
#include "cell.h"
#include "csv.h"
#include "file.h"

unsigned    WindowWidth, WindowHeight;
unsigned    VisibleRows, VisibleCols;
unsigned    CellHeight = 24, CellWidth = 80;
unsigned    CurRow, CurCol;
Table       TheTable;

COLORREF    color_grid = RGB(160, 160, 160);
COLORREF    color_cur_grid = RGB(0, 0, 0);
COLORREF    color_bg = RGB(224, 224, 224);
COLORREF    color_bg2 = RGB(200, 200, 220);
COLORREF    color_cur_bg = RGB(230, 230, 255);
HFONT       font_cell;

HDC         WindowBuffer;
HWND        TheWindow;
HWND        EditBox;

void DrawLine(HDC dc, int x, int y, int x2, int y2) {
    MoveToEx(dc, x, y, 0);
    LineTo(dc, x2, y2);
}

RECT get_cell_rect(unsigned row, unsigned col) {
    RECT rt;
    SetRect(&rt, col * CellWidth, row * CellHeight,
        (col + 1) * CellWidth, (row + 1) * CellHeight);
    return rt;
}

paint_table(HDC dc, Table *table) {
    unsigned row, col;
    
    SelectObject(dc, GetStockObject(DC_BRUSH));
    SelectObject(dc, GetStockObject(DC_PEN));
    SetDCBrushColor(dc, color_bg);
    SetDCPenColor(dc, color_grid);
    Rectangle(dc, 0, 0, WindowWidth, WindowHeight);
    
    SetDCBrushColor(dc, color_bg2);
    SelectObject(dc, GetStockObject(NULL_PEN));
    for (row = 0; row < VisibleRows; row += 2)
        Rectangle(dc, 0, row*CellHeight, WindowWidth, (row+1)*CellHeight);
    
    SelectObject(dc, GetStockObject(DC_PEN));
    for (col = 0; col < VisibleCols; col++)
        DrawLine(dc, col*CellWidth, 0, col*CellWidth, WindowHeight);
    
    /* Draw Cursor */
    {
        RECT rt = get_cell_rect(CurRow, CurCol);
        SetDCBrushColor(dc, color_cur_bg);
        SetDCPenColor(dc, color_cur_grid);
        Rectangle(dc, rt.left, rt.top, rt.right, rt.bottom);
    }
    
    SetBkMode(dc, TRANSPARENT);
    SelectFont(dc, font_cell);
    for (col = 0; col < VisibleCols; col++)
    for (row = 0; row < VisibleRows; row++) {
        Cell cell = try_cell(table, row, col);
        RECT rt = get_cell_rect(row, col);
        InflateRect(&rt, -3, -3);
        if (cell.len)
            DrawTextA(dc, cell.str, cell.len, &rt, DT_RIGHT);
    }
}

move_cursor(int row, int col) {
    CurRow = max(0, (int)CurRow + row);
    CurCol = max(0, (int)CurCol + col);
    InvalidateRect(TheWindow, 0, 0);
}

is_editing() { return GetWindowStyle(EditBox) & WS_VISIBLE; }

cancel_edit() {
    SetFocus(TheWindow);
    ShowWindow(EditBox, SW_HIDE);
}
end_edit() {
    if (is_editing()) {
        char buf[65536];
        int len = GetWindowTextLength(EditBox);
        GetWindowTextA (EditBox, buf, len + 1);
        set_cell(&TheTable, CurRow, CurCol, buf, len);
        cancel_edit();
        InvalidateRect(TheWindow, 0, 0);
    }
}
start_edit() {
    RECT rt = get_cell_rect(CurRow, CurCol);
    Cell cell; 
    if (is_editing()) end_edit();
    cell = try_cell(&TheTable, CurRow, CurCol); /* Get value after edit */
    SetWindowTextA(EditBox, cell.str);
    MoveWindow(EditBox, rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top, 0);
    ShowWindow(EditBox, SW_NORMAL);
    SetFocus(EditBox);
}

LRESULT CALLBACK
EditProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, UINT_PTR id, DWORD_PTR data) {
    if (msg == WM_CHAR)
        switch (wparam) {
        case VK_RETURN: end_edit(); move_cursor(1, 0); return 0;
        case VK_TAB: end_edit(); move_cursor(0, 1); return 0;
        case VK_ESCAPE: cancel_edit(); return 0;
        }
    return DefSubclassProc(hwnd, msg, wparam, lparam);
}

setup_resources(HWND hwnd) {
    HDC dc = GetDC(hwnd);
    
    SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);
    
    WindowBuffer = CreateCompatibleDC(dc);
    SelectBitmap(WindowBuffer, CreateCompatibleBitmap(dc, 1, 1));
    
    font_cell = CreateFont(12 * -GetDeviceCaps(dc, LOGPIXELSY)/72, 0,
        0, 0, FW_NORMAL,
        0, 0, 0, DEFAULT_CHARSET, CLIP_DEFAULT_PRECIS, OUT_DEFAULT_PRECIS,
        DRAFT_QUALITY, FF_DONTCARE, L"Constantia");
    ReleaseDC(0, dc);
    
    EditBox = CreateWindowEx(0, TEXT("EDIT"), TEXT(""),
        WS_CHILD | ES_RIGHT | ES_AUTOHSCROLL, 0, 0, 0, 0,
        hwnd, 0, GetModuleHandle(0), 0);
    SetWindowFont(EditBox, font_cell, 0);
    SetWindowSubclass(EditBox, EditProc, 0, 0);
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
    case WM_SIZE:
        WindowWidth = LOWORD(lparam);
        WindowHeight = HIWORD(lparam);
        VisibleRows = WindowHeight / CellHeight;
        VisibleCols = WindowWidth / CellWidth;
        DeleteBitmap(SelectBitmap(WindowBuffer,
            CreateCompatibleBitmap(WindowBuffer, WindowWidth, WindowHeight)));
        return 0;
    case WM_SETFOCUS:
        if (is_editing())
            SetFocus(EditBox);
        return 0;
    case WM_KEYDOWN:
        switch (wparam) {
        case VK_UP: move_cursor(-1, 0); break;
        case VK_DOWN: move_cursor(1, 0); break;
        case VK_LEFT: move_cursor(0, -1); break;
        case VK_RIGHT: move_cursor(0, 1); break;
        
        case VK_F2: start_edit(); break;
        
        case VK_RETURN: move_cursor(1, 0); break;
        case VK_TAB: move_cursor(0, 1); break;
        }
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_CREATE:
        setup_resources(hwnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd,msg,wparam,lparam);
}

int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmd, int show) {
    MSG         msg;
    WNDCLASS    wc = { CS_DBLCLKS|CS_HREDRAW|CS_VREDRAW,
        WndProc, 0, 0, 0, LoadIcon(0, IDI_APPLICATION),
        LoadCursor(0, IDC_ARROW), (HBRUSH)(COLOR_WINDOW+1), 0,
        TEXT("Window")};
    RegisterClass(&wc);
    TheWindow = CreateWindowEx(WS_EX_LAYERED, TEXT("Window"), TEXT(""),
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT,CW_USEDEFAULT,
        CW_USEDEFAULT,CW_USEDEFAULT,
        0, 0, GetModuleHandle(0), 0);
    
    /* Open command line file */
    {
        unsigned len;
        int argc;
        short **argv = CommandLineToArgvW(GetCommandLineW(), &argc);
        char *data = argv[1]? open_file(argv[1], &len): 0;
        if (data) {
            read_csv(&TheTable, data, data + len);
            close_file(data);
        }
    }
    
    while (GetMessage(&msg,0,0,0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
main() { return WinMain(0,0,0,0); }
