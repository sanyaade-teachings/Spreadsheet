unsigned    WindowWidth, WindowHeight;
unsigned    VisibleRows, VisibleCols;
unsigned    CellHeight = 24, CellWidth = 80;

COLORREF    color_grid = RGB(160, 160, 160);
COLORREF    color_cur_grid = RGB(0, 0, 0);
COLORREF    color_bg = RGB(224, 224, 224);
COLORREF    color_bg2 = RGB(200, 200, 220);
COLORREF    color_cur_bg = RGB(230, 230, 255);
HFONT       font_cell;

HDC         WindowBuffer;

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
    return 0;
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

wm_size(HWND hwnd, unsigned width, unsigned height) {
    HDC dc = GetDC(hwnd);
    WindowWidth = width;
    WindowHeight = height;
    VisibleRows = WindowHeight / CellHeight;
    VisibleCols = WindowWidth / CellWidth;
    DeleteBitmap(SelectBitmap(WindowBuffer,
        CreateCompatibleBitmap(dc, WindowWidth, WindowHeight)));
    ReleaseDC(hwnd, dc);
}

init_ui_display(HWND hwnd) {
    HDC dc = GetDC(hwnd);
    
    SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);
    
    WindowBuffer = CreateCompatibleDC(dc);
    SelectBitmap(WindowBuffer, CreateCompatibleBitmap(dc, 1, 1));
    
    font_cell = CreateFont(12 * -GetDeviceCaps(dc, LOGPIXELSY)/72, 0,
        0, 0, FW_NORMAL,
        0, 0, 0, DEFAULT_CHARSET, CLIP_DEFAULT_PRECIS, OUT_DEFAULT_PRECIS,
        DRAFT_QUALITY, FF_DONTCARE, L"Constantia");
    ReleaseDC(0, dc);
}
