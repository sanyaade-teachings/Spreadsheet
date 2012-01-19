unsigned    CellHeight = 24;
#define     LastRow (tui->first_row + tui->visible_rows)
#define     LastCol (tui->first_col + tui->visible_cols)

COLORREF    color_grid = RGB(160, 160, 160);
COLORREF    color_cur_grid = RGB(0, 0, 0);
COLORREF    color_bg = RGB(224, 224, 224);
COLORREF    color_bg2 = RGB(200, 200, 220);
COLORREF    color_cur_bg = RGB(230, 230, 255);
HFONT       font_cell;

get_col_max_width(TableUI *tui, unsigned col);
paint_cell(TableUI *tui, unsigned row, unsigned col, unsigned is_drawn);

unsigned get_cell_x(TableUI *tui, unsigned col) {
    return tui->col_pos[col] - tui->col_pos[tui->first_col];
}
unsigned get_cell_y(TableUI *tui, unsigned row) {
    return (row - tui->first_row) * CellHeight;
}

/* Returns 1 if near resizing bar */
get_cell_under(TableUI *tui, unsigned x, unsigned y, unsigned *rowp, unsigned *colp, unsigned *is_resizep) {
    unsigned r,c;
    r = (y / CellHeight) + tui->first_row;
    if (rowp) *rowp = r;
    for (c = tui->first_col; tui->col_pos[c] < x; c++);
    if (colp) *colp = c - 1;
    if (is_resizep) *is_resizep = (r == tui->first_row && tui->col_pos[c] - x < 8);
}

calc_visible_fields(TableUI *tui) {
    tui->visible_rows = tui->height / CellHeight;    
    for (tui->visible_cols = 0; get_cell_x(tui,tui->visible_cols+1) <= tui->width; tui->visible_cols++);    
}

resize_col(TableUI *tui, unsigned col, int dx) {
    unsigned i;
    for (i = col + 1; i < 65536; i++)
        tui->col_pos[i] += dx;
    calc_visible_fields(tui); /* Field's may have shrunk or grown */
}

auto_resize_col(TableUI *tui, unsigned col) {
    unsigned fit = get_col_max_width(tui, col);
    fit = clamp(MIN_FIT_WIDTH, fit, MAX_FIT_WIDTH);
    resize_col(tui, col, get_cell_x(tui,col) + fit - get_cell_x(tui,col + 1));
    redraw_rows(tui, 0, -1);
}

reset_col_sizes(TableUI *tui) {
    unsigned i;
    tui->col_pos[0] = 0;
    for (i = 1; i < sizeof tui->col_pos / sizeof *tui->col_pos; i++)
        tui->col_pos[i] = tui->col_pos[i - 1] + 80;
}

scroll(TableUI *tui, int row, int col) {
    tui->first_row = max(0, (int)tui->first_row + row);
    tui->first_col = max(0, (int)tui->first_col + col);
    redraw_rows(tui, 0, -1);
}

snap_to_cursor(TableUI *tui) {
    /* Snap to first row if before screne; last if after. */
    unsigned x = 0;
    if (tui->cur_row < tui->first_row) tui->first_row = tui->cur_row, x = 1;
    if (LastRow <= tui->cur_row) tui->first_row = max(0, tui->cur_row - tui->visible_rows + 1), x = 1;
    if (tui->cur_col < tui->first_col) tui->first_col = tui->cur_col, x = 1;
    if (LastCol <= tui->cur_col) tui->first_col = max(0, tui->cur_col - tui->visible_cols + 1), x = 1;
    
    if (x) redraw_rows(tui, 0, -1);
}

void DrawLine(HDC dc, int x, int y, int x2, int y2) {
    MoveToEx(dc, x, y, 0);
    LineTo(dc, x2, y2);
}

/* Get rectangle that cell occupies or empty if COMPLETELY off screen */
RECT get_cell_rect(TableUI *tui, unsigned row, unsigned col) {
    RECT rt;
    
    if (row < tui->first_row || LastRow < row || col < tui->first_col || LastCol < col)
        SetRect(&rt, 0, 0, 0, 0);
    else
        SetRect(&rt,
            get_cell_x(tui,col),     get_cell_y(tui,row),
            get_cell_x(tui,col + 1), get_cell_y(tui,row + 1));
    return rt;
}

get_col_max_width(TableUI *tui, unsigned col) {
    unsigned r, width = 0;
    for (r = 0; r < row_count(tui->table); r++)
        width = max(width, paint_cell(tui, r, col, 0));
    return width + 6;
}

redraw_rows(TableUI *tui, unsigned lo, unsigned hi) {
    RECT lor, hir, bad;
    unsigned x = lo;
    
    lo = min(lo, hi);
    hi = max(x, hi);
    if (hi < tui->first_row || LastRow < lo);      /* Completely off screen */
    else {
        if (lo < tui->first_row) lo = tui->first_row;      /* Above before screen */
        if (LastRow < hi) hi = LastRow;          /* Ends after screen */
        lor = get_cell_rect(tui, lo, tui->first_col);
        hir = get_cell_rect(tui, hi, tui->first_col);
        SetRect(&bad, 0, lor.top, tui->width, hir.bottom);
        InvalidateRect(tui->window, &bad, 0);
    }
    return 0;
}

/* Returns width of content if is_drawn is passed; Empty cells return 0 */
paint_cell(TableUI *tui, unsigned row, unsigned col, unsigned is_drawn) {
    Cell cell = try_cell(tui->table, row, col);
    RECT rt = get_cell_rect(tui, row, col);
    InflateRect(&rt, -3, -3);
    if (!cell.len) return 0;
    DrawTextA(tui->bitmap, cell.str, cell.len, &rt,
        DT_NOPREFIX | (is_drawn? 0: DT_CALCRECT));
    return rt.right - rt.left;
}

paint_table(TableUI *tui) {
    HDC     dc = tui->bitmap;
    Table   *table = tui->table;
    unsigned row, col;
    RECT    rt;
    
    SelectObject(dc, GetStockObject(DC_BRUSH));
    SelectObject(dc, GetStockObject(DC_PEN));
    SetDCBrushColor(dc, color_bg);
    SetDCPenColor(dc, color_grid);
    Rectangle(dc, 0, 0, tui->width, tui->height);
    
    /* Draw Alternating Rows */
    SetDCBrushColor(dc, color_bg2);
    SelectObject(dc, GetStockObject(NULL_PEN));
    for (row = tui->first_row; row <= LastRow; row += 2)
        Rectangle(dc, 0, get_cell_y(tui,row), tui->width, get_cell_y(tui,row + 1));
    
    /* Draw Grid */
    SelectObject(dc, GetStockObject(DC_PEN));
    for (col = tui->first_col; col <= LastCol; col++)
        DrawLine(dc, get_cell_x(tui,col), 0, get_cell_x(tui,col), tui->height);
    
    /* Draw Cursor & selection rectangle */
    SetDCBrushColor(dc, color_cur_bg);
    SetDCPenColor(dc, color_cur_grid);
    if (tui->is_selecting)
        Rectangle(dc,
            (int)get_cell_x(tui,SelStartCol), (int)get_cell_y(tui,SelStartRow),
            (int)get_cell_x(tui,SelEndCol), (int)get_cell_y(tui,SelEndRow));
    rt = get_cell_rect(tui, tui->cur_row, tui->cur_col);
    Rectangle(dc, rt.left, rt.top, rt.right, rt.bottom);
    
    /* Draw cells; Draw one more than fully visible to get partial cells */
    SetBkMode(dc, TRANSPARENT);
    SelectFont(dc, font_cell);
    for (col = tui->first_col; col <= LastCol; col++)
    for (row = tui->first_row; row <= LastRow; row++)
        paint_cell(tui, row, col, 1);
}

wm_size(TableUI *tui, unsigned width, unsigned height) {
    HDC dc = GetDC(tui->window);
    tui->width = width;
    tui->height = height;
    calc_visible_fields(tui);
    DeleteBitmap(SelectBitmap(tui->bitmap,
        CreateCompatibleBitmap(dc, tui->width, tui->height)));
    ReleaseDC(tui->window, dc);
}

init_ui_display(TableUI *tui) {
    HDC dc = GetDC(tui->window);
    
    reset_col_sizes(tui);
    
    SetLayeredWindowAttributes(tui->window, 0, 255, LWA_ALPHA);
    
    tui->bitmap = CreateCompatibleDC(dc);
    SelectBitmap(tui->bitmap, CreateCompatibleBitmap(dc, 1, 1));
    
    font_cell = CreateFont(10 * -GetDeviceCaps(dc, LOGPIXELSY)/72, 0,
        0, 0, FW_NORMAL,
        0, 0, 0, DEFAULT_CHARSET, CLIP_DEFAULT_PRECIS, OUT_DEFAULT_PRECIS,
        DRAFT_QUALITY, FF_DONTCARE, L"Constantia");
    ReleaseDC(tui->window, dc);
}
