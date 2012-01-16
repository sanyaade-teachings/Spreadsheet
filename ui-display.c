unsigned    WindowWidth, WindowHeight;
unsigned    VisibleRows, VisibleCols;
unsigned    FirstRow, FirstCol;
unsigned    CellHeight = 24;
#define     LastRow (FirstRow + VisibleRows)
#define     LastCol (FirstCol + VisibleCols)

COLORREF    color_grid = RGB(160, 160, 160);
COLORREF    color_cur_grid = RGB(0, 0, 0);
COLORREF    color_bg = RGB(224, 224, 224);
COLORREF    color_bg2 = RGB(200, 200, 220);
COLORREF    color_cur_bg = RGB(230, 230, 255);
HFONT       font_cell;

HDC         WindowBuffer;

unsigned    ColXs[65536];

unsigned get_cell_x(unsigned col) {
    return ColXs[col];
}
unsigned get_cell_y(unsigned row) {
    return (row - FirstRow) * CellHeight;
}

/* Returns 1 if near resizing bar */
get_cell_under(unsigned x, unsigned y, unsigned *rowp, unsigned *colp, unsigned *is_resizep) {
    unsigned r,c;
    r = (y / CellHeight) + FirstRow;
    if (rowp) *rowp = r;
    for (c = FirstCol; ColXs[c] < x; c++);
    if (colp) *colp = c - 1;
    if (is_resizep) *is_resizep = (r == FirstRow && ColXs[c] - x < 8);
}

scroll(int row, int col) {
    unsigned orow = FirstRow, ocol = FirstCol;
    FirstRow = max(0, (int)FirstRow + row);
    FirstCol = max(0, (int)FirstCol + col);
    redraw_rows(0, -1);
}

snap_to_cursor() {
    /* Snap to first row if before screne; last if after. */
    unsigned x = 0;
    if (CurRow < FirstRow) FirstRow = CurRow, x = 1;
    if (LastRow <= CurRow) FirstRow = max(0, CurRow - VisibleRows + 1), x = 1;
    if (CurCol < FirstCol) FirstCol = CurCol, x = 1;
    if (LastCol <= CurCol) FirstCol = max(0, CurCol - VisibleCols + 1), x = 1;
    
    if (x) redraw_rows(0, -1);
}

void DrawLine(HDC dc, int x, int y, int x2, int y2) {
    MoveToEx(dc, x, y, 0);
    LineTo(dc, x2, y2);
}

/* Get rectangle that cell occupies or empty if COMPLETELY off screen */
RECT get_cell_rect(unsigned row, unsigned col) {
    RECT rt;
    
    if (row < FirstRow || LastRow < row || col < FirstCol || LastCol < col)
        SetRect(&rt, 0, 0, 0, 0);
    else
        SetRect(&rt,
            get_cell_x(col),     get_cell_y(row),
            get_cell_x(col + 1), get_cell_y(row + 1));
    return rt;
}

get_col_max_width(unsigned col) {
    unsigned r, width = 0;
    for (r = 0; r < row_count(&TheTable); r++)
        width = max(width, paint_cell(WindowBuffer, &TheTable, r, col, 0));
    return width + 6;
}

redraw_rows(unsigned lo, unsigned hi) {
    RECT lor, hir, bad;
    unsigned x = lo;
    
    lo = min(lo, hi);
    hi = max(x, hi);
    if (hi < FirstRow || LastRow < lo);      /* Completely off screen */
    else {
        if (lo < FirstRow) lo = FirstRow;      /* Above before screen */
        if (LastRow < hi) hi = LastRow - 1;      /* Ends after screen */
        lor = get_cell_rect(lo, FirstCol);
        hir = get_cell_rect(hi, FirstCol);
        SetRect(&bad, 0, lor.top, WindowWidth, hir.bottom);
        InvalidateRect(TheWindow, &bad, 0);
    }
    return 0;
}

/* Returns width of content if is_drawn is passed; Empty cells return 0 */
paint_cell(HDC dc, Table *table, unsigned row, unsigned col, unsigned is_drawn) {
    Cell cell = try_cell(table, row, col);
    RECT rt = get_cell_rect(row, col);
    InflateRect(&rt, -3, -3);
    if (!cell.len) return 0;
    DrawTextA(dc, cell.str, cell.len, &rt,
        DT_NOPREFIX | DT_RIGHT | (is_drawn? 0: DT_CALCRECT));
    return rt.right - rt.left;
}

paint_table(HDC dc, Table *table) {
    unsigned row, col;
    RECT    rt, rt2;
    
    SelectObject(dc, GetStockObject(DC_BRUSH));
    SelectObject(dc, GetStockObject(DC_PEN));
    SetDCBrushColor(dc, color_bg);
    SetDCPenColor(dc, color_grid);
    Rectangle(dc, 0, 0, WindowWidth, WindowHeight);
    
    SetDCBrushColor(dc, color_bg2);
    SelectObject(dc, GetStockObject(NULL_PEN));
    for (row = 0; row <= VisibleRows; row += 2)
        Rectangle(dc, 0, row*CellHeight, WindowWidth, (row+1)*CellHeight);
    
    /* Draw Grid */
    SelectObject(dc, GetStockObject(DC_PEN));
    for (col = 0; col <= VisibleCols; col++)
        DrawLine(dc, get_cell_x(col), 0, get_cell_x(col), WindowHeight);
    
    /* Draw Cursor & selection rectangle */
    SetDCBrushColor(dc, color_cur_bg);
    SetDCPenColor(dc, color_cur_grid);
    if (is_selecting) {
        rt = get_cell_rect(CurRow, CurCol);
        rt2 = get_cell_rect(AnchorRow, AnchorCol);
        UnionRect(&rt, &rt, &rt2);
        Rectangle(dc, rt.left, rt.top, rt.right, rt.bottom);
    }
    rt = get_cell_rect(CurRow, CurCol);
    Rectangle(dc, rt.left, rt.top, rt.right, rt.bottom);
    
    /* Draw cells; Draw one more than fully visible to get partial cells */
    SetBkMode(dc, TRANSPARENT);
    SelectFont(dc, font_cell);
    for (col = FirstCol; col <= LastCol; col++)
    for (row = FirstRow; row <= LastRow; row++)
        paint_cell(dc, table, row, col, 1);
}

wm_size(HWND hwnd, unsigned width, unsigned height) {
    HDC dc = GetDC(hwnd);
    WindowWidth = width;
    WindowHeight = height;
    VisibleRows = WindowHeight / CellHeight;
    
    for (VisibleCols = 0; get_cell_x(VisibleCols+1) <= WindowWidth; VisibleCols++);    
    
    DeleteBitmap(SelectBitmap(WindowBuffer,
        CreateCompatibleBitmap(dc, WindowWidth, WindowHeight)));
    ReleaseDC(hwnd, dc);
}

init_ui_display(HWND hwnd) {
    unsigned i;
    HDC dc = GetDC(hwnd);
    
    ColXs[0] = 0;
    for (i = 1; i < sizeof ColXs / sizeof *ColXs; i++)
        ColXs[i] = ColXs[i - 1] + 80;
    
    SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);
    
    WindowBuffer = CreateCompatibleDC(dc);
    SelectBitmap(WindowBuffer, CreateCompatibleBitmap(dc, 1, 1));
    
    font_cell = CreateFont(12 * -GetDeviceCaps(dc, LOGPIXELSY)/72, 0,
        0, 0, FW_NORMAL,
        0, 0, 0, DEFAULT_CHARSET, CLIP_DEFAULT_PRECIS, OUT_DEFAULT_PRECIS,
        DRAFT_QUALITY, FF_DONTCARE, L"Constantia");
    ReleaseDC(0, dc);
}
