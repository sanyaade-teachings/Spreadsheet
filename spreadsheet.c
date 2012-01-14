#define _WIN32_WINNT    0x0501
#define WIN32_LEAN_AND_MEAN
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <commdlg.h>
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

TCHAR           open_dlg_fn[MAX_PATH];
OPENFILENAME    open_dlg = {
                sizeof open_dlg, 0, 0,
                L"Comma Seperated Values (*.csv)\0*.csv\0"
                    L"Text File (*.txt)\0*.txt\0",
                0, 0, 0, open_dlg_fn, MAX_PATH, 0, 0,
                0, 0,
                OFN_FILEMUSTEXIST | OFN_OVERWRITEPROMPT, };

#define IsShiftDown() (GetKeyState(VK_SHIFT) < 0)
#define IsCtrlDown() (GetKeyState(VK_CONTROL) < 0)

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

redraw_rows(unsigned lo, unsigned hi) {
    RECT lor, hir;
    RECT bad;
    
    if (VisibleRows < hi) hi = VisibleRows;
    lor = get_cell_rect(lo, 0);
    hir = get_cell_rect(hi, 0);
    
    SetRect(&bad, 0, min(lor.top, hir.top),
        WindowWidth, max(lor.bottom, hir.bottom));
    InvalidateRect(TheWindow, &bad, 0);
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
            DrawTextA(dc, cell.str, cell.len, &rt,
                DT_NOPREFIX | DT_RIGHT);
    }
}

jump_cursor(unsigned row, unsigned col) {
    redraw_rows(CurRow, CurRow);              /* Clear Cursor */
    CurRow = row;
    CurCol = col;
    redraw_rows(CurRow, CurRow);               /* Draw Cursor */
}

move_cursor(int row, int col) {
    jump_cursor(max(0, (int)CurRow + row), max(0, (int)CurCol + col));
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
        redraw_rows(CurRow, CurRow);
    }
}
start_edit(int edit_existing) {
    RECT rt = get_cell_rect(CurRow, CurCol), rt_cont = rt;    
    
    if (is_editing()) end_edit();
    
    if (edit_existing) {
        Cell cell = try_cell(&TheTable, CurRow, CurCol);
        SetWindowTextA(EditBox, cell.str);
        
        /* Resize Edit to fit content; at least as big as the cell */
        DrawTextA(WindowBuffer, cell.str, cell.len, &rt_cont,
            DT_NOPREFIX | DT_RIGHT | DT_CALCRECT | DT_EDITCONTROL);
        UnionRect(&rt, &rt, &rt_cont);
    } else
        SetWindowText(EditBox, L"");
    
    MoveWindow(EditBox, rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top, 0);
    ShowWindow(EditBox, SW_NORMAL);
    SetFocus(EditBox);
}

open_csv(TCHAR *fn) {
    char *data;
    unsigned len;
    if (!fn) return 0;
    if (data = open_file(fn, &len)) {
        read_csv(&TheTable, data, data + len);
        close_file(data);
    }
    return !!data;
}

save_csv(TCHAR *fn) {
    FILE *f = _wfopen(fn, L"wb");
    if (f) {
        write_csv(f, TheTable);
        fclose(f);
    }
    return !!f;
}

LRESULT CALLBACK
EditProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, UINT_PTR id, DWORD_PTR data) {
    if (msg == WM_CHAR)
        switch (wparam) {
        case VK_RETURN:
            if (IsCtrlDown()) break;
            end_edit();
            move_cursor(1, 0);
            return 0;
        case VK_TAB:
            if (IsCtrlDown()) break;
            end_edit();
            move_cursor(0, 1);
            return 0;
        case VK_ESCAPE:
            cancel_edit();
            return 0;
        }
    return DefSubclassProc(hwnd, msg, wparam, lparam);
}

setup_resources(HWND hwnd) {
    HDC dc = GetDC(hwnd);
    
    SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);
    
    open_dlg.hwndOwner = hwnd;
    
    WindowBuffer = CreateCompatibleDC(dc);
    SelectBitmap(WindowBuffer, CreateCompatibleBitmap(dc, 1, 1));
    
    font_cell = CreateFont(12 * -GetDeviceCaps(dc, LOGPIXELSY)/72, 0,
        0, 0, FW_NORMAL,
        0, 0, 0, DEFAULT_CHARSET, CLIP_DEFAULT_PRECIS, OUT_DEFAULT_PRECIS,
        DRAFT_QUALITY, FF_DONTCARE, L"Constantia");
    ReleaseDC(0, dc);
    
    EditBox = CreateWindowEx(0, TEXT("EDIT"), TEXT(""),
        WS_CHILD | ES_RIGHT | ES_AUTOHSCROLL | ES_AUTOVSCROLL
        | ES_MULTILINE | ES_WANTRETURN, 0, 0, 0, 0,
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
        ps.hdc = GetDC(hwnd);
        DeleteBitmap(SelectBitmap(WindowBuffer,
            CreateCompatibleBitmap(ps.hdc, WindowWidth, WindowHeight)));
        ReleaseDC(hwnd, ps.hdc);
        return 0;
    case WM_SETFOCUS:
        if (is_editing())
            SetFocus(EditBox);
        return 0;
    case WM_CHAR:
        switch (wparam) {
        
        case 'L' - 'A' + 1:
            if (IsShiftDown()) {
                delete_row(&TheTable, CurRow);
                redraw_rows(CurRow, -1);
            } else {
                clear_row(&TheTable, CurRow);
                redraw_rows(CurRow, CurRow);
            }
            break;
            
        case 'N' - 'A' + 1:
            open_dlg_fn[0] = 0;
            delete_table(&TheTable);
            redraw_rows(0, -1);
            break;
            
        case 'O' - 'A' + 1:
            if (GetOpenFileName(&open_dlg)) {
                delete_table(&TheTable);
                if (!open_csv(open_dlg_fn))
                    MessageBox(hwnd, L"Could not open the file", L"Error", MB_OK);
                InvalidateRect(hwnd, 0, 0);
            }
            return 0;
        
        case 'S' - 'A' + 1:
            if (open_dlg_fn[0] || GetSaveFileName(&open_dlg))
                if (!save_csv(open_dlg_fn))
                    MessageBox(hwnd, L"Could not save the file", L"Error", MB_OK);
            return 0;
        
        case VK_RETURN: move_cursor(1, 0); break;
        case VK_TAB: move_cursor(0, 1); break;
        
        default:
            start_edit(0);
            SendMessage(EditBox, WM_CHAR, wparam, lparam); /* Don't drop the char */
            break;
        }
        break;
    case WM_KEYDOWN:
        switch (wparam) {
        case VK_UP: move_cursor(-1, 0); break;
        case VK_DOWN: move_cursor(1, 0); break;
        case VK_LEFT: move_cursor(0, -1); break;
        case VK_RIGHT: move_cursor(0, 1); break;
        
        case VK_F2: start_edit(1); break;
        
        case VK_HOME:
            if (IsCtrlDown())
                jump_cursor(0, CurCol);
            else
                jump_cursor(CurRow, 0);
            break;
            
        case VK_END:
            if (IsCtrlDown())
                jump_cursor(row_count(&TheTable), CurCol);
            else
                jump_cursor(CurRow, col_count(&TheTable, CurRow));
            break;
        
        case VK_DELETE:
            if (IsShiftDown())
                delete_cell(&TheTable, CurRow, CurCol);
            else
                clear_cell(&TheTable, CurRow, CurCol);
            redraw_rows(CurRow, CurRow);
            break;
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
    int         argc;
    short       **argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    MSG         msg;
    WNDCLASS    wc = { CS_DBLCLKS|CS_HREDRAW|CS_VREDRAW,
        WndProc, 0, 0, 0, LoadIcon(0, IDI_APPLICATION),
        LoadCursor(0, IDC_ARROW), (HBRUSH)(COLOR_WINDOW+1), 0,
        TEXT("Window")};
    RegisterClass(&wc);
    InitCommonControls();
    TheWindow = CreateWindowEx(WS_EX_LAYERED, TEXT("Window"), TEXT(""),
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT,CW_USEDEFAULT,
        CW_USEDEFAULT,CW_USEDEFAULT,
        0, 0, GetModuleHandle(0), 0);
    
    /* Open command line file */
    if (argv[1] && !open_csv(argv[1]))
        MessageBox(TheWindow, L"Could not open the file", L"Error", MB_OK);
    else
        wcscpy(open_dlg_fn, argv[1]);
    
    while (GetMessage(&msg,0,0,0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
main() { return WinMain(0,0,0,0); }
