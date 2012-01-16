#define MOUSE_SCROLL_STEP_X 75
#define MOUSE_SCROLL_STEP_Y 25

enum {
    MOUSE_MODE_NORMAL,
    MOUSE_MODE_RESIZING,
    MOUSE_MODE_SCROLLING
};

unsigned    mouse_mode;
POINT       mouse_scroll_anchor;
unsigned    resizing_col;

HCURSOR     cursor_resize_col;
HCURSOR     cursor_arrow;
HCURSOR     cursor_scrolling;

OPENFILENAME    open_dlg = {
    sizeof open_dlg, 0, 0,
    L"Comma Seperated Values (*.csv)\0*.csv\0"
        L"Text File (*.txt)\0*.txt\0",
    0, 0, 0, TheFilename, MAX_PATH, 0, 0, 0, 0,
    OFN_FILEMUSTEXIST | OFN_OVERWRITEPROMPT, };

#define IsShiftDown() (GetKeyState(VK_SHIFT) < 0)
#define IsCtrlDown() (GetKeyState(VK_CONTROL) < 0)

resize_column(unsigned col, int dx) {
    unsigned i;
    for (i = col + 1; i < 65536; i++)
        ColXs[i] += dx;
    calc_visible_fields(); /* Field's may have shrunk or grown */
}

auto_resize_column(unsigned col) {
    unsigned fit = get_col_max_width(col);
    fit = clamp(MIN_FIT_WIDTH, fit, MAX_FIT_WIDTH);
    resize_column(col, get_cell_x(col) + fit - get_cell_x(col + 1));
    redraw_rows(0, -1);
}

is_editing() {
    return GetWindowStyle(EditBox) & WS_VISIBLE;
}

cancel_edit() {
    SetFocus(TheWindow);
    ShowWindow(EditBox, SW_HIDE);
}

end_edit() {
    if (is_editing()) {
        char buf[65536];           /* An EDIT control's limit */
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
    
    MoveWindow(EditBox, rt.left, rt.top,
        rt.right - rt.left, rt.bottom - rt.top, 0);
    ShowWindow(EditBox, SW_NORMAL);
    SetFocus(EditBox);
}

LRESULT CALLBACK
EditProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, UINT_PTR id, DWORD_PTR data) {
    if (msg == WM_CHAR)
        switch (wparam) {
        case VK_RETURN:
            if (IsCtrlDown()) break;
            command(CmdCommitEdit);
            return 0;
        case VK_TAB:
            if (IsCtrlDown()) break;
            command(CmdTab);
            command(CmdCommitEdit);
            return 0;
        case VK_ESCAPE:
            command(CmdCancelEdit);
            return 0;
        }
    return DefSubclassProc(hwnd, msg, wparam, lparam);
}

wm_char(HWND hwnd, unsigned wparam) {
    switch (wparam) {
    
    case 'L' - 'A' + 1:                         /* Delete Row */
        if (IsShiftDown()) command(CmdDeleteRow);
        else command(CmdClearRow);
        break;
        
    case 'N' - 'A' + 1:                           /* New File */
        command(CmdClearFile);
        break;
        
    case 'O' - 'A' + 1:                          /* Open file */
        if (GetOpenFileName(&open_dlg))        
            command(CmdOpenFile);
        break;
    
    case 'S' - 'A' + 1:                          /* Save File */
        if (TheFilename[0] || GetSaveFileName(&open_dlg))
            command(CmdSaveFile);
        break;
    
    case 'C' - 'A' + 1:                               /* Copy */
        command(CmdCopy);
        break;
    
    case 'X' - 'A' + 1:                                /* Cut */
        command(IsShiftDown()? CmdCutDelete: CmdCutClear);
        break;
    
    case 'V' - 'A' + 1:                              /* Paste */
        command(CmdPaste);
        break;
        
    case VK_RETURN:                                  /* Enter */
        command(IsShiftDown()? CmdUnReturn: CmdReturn);
        break;
        
    case VK_TAB:                                       /* Tab */
        command(IsShiftDown()? CmdUnTab: CmdTab);
        break;
    
    default:                                /* Auto-edit cell */
        /* Don't drop the char; forward it to the editor */
        command(CmdEditCellClear);
        SendMessage(EditBox, WM_CHAR, wparam, 0); 
        break;
    }
    return 1;
}

wm_keydown(HWND hwnd, unsigned wparam) {
    switch (wparam) {
    
    case VK_UP:
        if (IsCtrlDown())
            command(CmdScrollUp);
        else {
            command(IsShiftDown()? CmdSetAnchor: CmdClearAnchor);
            command(CmdMoveUp);
        }
        break;
    case VK_DOWN:
        if (IsCtrlDown())
            command(CmdScrollDown);
        else  {
            command(IsShiftDown()? CmdSetAnchor: CmdClearAnchor);
            command(CmdMoveDown);
        }
        break;
    case VK_LEFT:
        if (IsCtrlDown())
            command(CmdScrollLeft);
        else {
            command(IsShiftDown()? CmdSetAnchor: CmdClearAnchor);
            command(CmdMoveLeft);
        }
        break;
    case VK_RIGHT:
        if (IsCtrlDown())
            command(CmdScrollRight);
        else {
            command(IsShiftDown()? CmdSetAnchor: CmdClearAnchor);
            command(CmdMoveRight);
        }
        break;
    
    case VK_F2:
        command(CmdEditCell);
        break;
    
    case VK_HOME:
        command(IsShiftDown()? CmdSetAnchor: CmdClearAnchor);
        command(IsCtrlDown()? CmdHomeCol: CmdHomeRow);    
        break;
        
    case VK_END:
        command(IsShiftDown()? CmdSetAnchor: CmdClearAnchor);
        command(IsCtrlDown()? CmdEndCol: CmdEndRow);    
        break;
    
    case VK_DELETE:
        command(IsShiftDown()? CmdDeleteCell: CmdClearCell) ;
        break;
    
    case VK_OEM_1: /* Semicolon */
        if (IsCtrlDown()) {               /* Insert Date/Time */
            command(IsShiftDown()? CmdInsertDateTime: CmdInsertDate);
            break;
        }
        return 0;

    case VK_OEM_PERIOD: /* . */
        if (IsCtrlDown()) {
            command(IsShiftDown()? CmdInsertCell: CmdInsertRow);
            break;
        }
        return 0;
    }
    return 1;
}


void CALLBACK ScrollTimerProc(HWND hwnd, UINT msg, UINT_PTR id, DWORD time) {
    DWORD pos = GetMessagePos();
    POINT pt;
    pt.y = GET_Y_LPARAM(pos);
    pt.x = GET_X_LPARAM(pos);
    ScreenToClient(hwnd, &pt);
    scroll(
        (pt.y - mouse_scroll_anchor.y) / MOUSE_SCROLL_STEP_Y,
        (pt.x - mouse_scroll_anchor.x) / MOUSE_SCROLL_STEP_X);
}

exit_mouse_mode(HWND hwnd) {
    switch (mouse_mode) {
    
    case MOUSE_MODE_SCROLLING:
        KillTimer(hwnd, 1);
        ReleaseCapture();
        break;
    
    case MOUSE_MODE_RESIZING:
        SetCursor(cursor_arrow);
        ReleaseCapture();
        break;
    }
    mouse_mode = 0;
}

enter_mouse_mode(HWND hwnd, unsigned mode, unsigned x, unsigned y) {
    unsigned old_mode = mouse_mode;
        
    exit_mouse_mode(hwnd);
    
    if (!old_mode)/* Only changed modes if coming from normal */
        switch (mouse_mode = mode) {
        
        case MOUSE_MODE_SCROLLING:
            mouse_scroll_anchor.x = x;
            mouse_scroll_anchor.y = y;
            SetCursor(cursor_scrolling);
            SetTimer(hwnd, 1, 100, ScrollTimerProc);
            SetCapture(hwnd);
            break;
        
        case MOUSE_MODE_RESIZING:
            resizing_col = y;
            SetCapture(hwnd);
            break;
        
        }
}

wm_mbuttondown(HWND hwnd, unsigned x, unsigned y) {
    enter_mouse_mode(hwnd, MOUSE_MODE_SCROLLING, x, y);
}
wm_mbuttonup(HWND hwnd, unsigned x, unsigned y) {
    exit_mouse_mode(hwnd);
}

wm_lbuttondown(HWND hwnd, unsigned x, unsigned y) {
    unsigned row, col, is_resize;
    
    exit_mouse_mode(hwnd);
    get_cell_under(x, y, &row, &col, &is_resize);
    if (is_resize)
        enter_mouse_mode(hwnd, MOUSE_MODE_RESIZING, 0, col);
    else {
        command(IsShiftDown()? CmdSetAnchor: CmdClearAnchor);
        jump_cursor(row, col);
    }
}

wm_lbuttonup(HWND hwnd, unsigned x, unsigned y) {
    exit_mouse_mode(hwnd);
}

wm_mousemove(HWND hwnd, unsigned x, unsigned y) {
    if (mouse_mode == MOUSE_MODE_RESIZING) {
        resize_column(resizing_col, x - get_cell_x(resizing_col + 1));
        redraw_rows(0, -1);
    } else {
        unsigned row, col, is_resize;
        get_cell_under(x, y, &row, &col, &is_resize);
        if (is_resize)
            SetCursor(cursor_resize_col);
        else if (GetCursor() == cursor_resize_col)
            SetCursor(cursor_arrow);
    }
}

wm_lbuttondblclk(HWND hwnd, unsigned x, unsigned y) {
    unsigned row, col, is_resize;
    
    exit_mouse_mode(hwnd);
    
    get_cell_under(x, y, &row, &col, &is_resize);
    if (is_resize)
        auto_resize_column(col);
    else {
        command(IsShiftDown()? CmdSetAnchor: CmdClearAnchor);
        jump_cursor(row, col);
        command(CmdEditCell);
        start_edit(1);
    }
}

wm_mousewheel(HWND hwnd, int delta) {
    if (!mouse_mode)
        scroll(delta / -WHEEL_DELTA, 0);
}

wm_dropfiles(HWND hwnd, HDROP drop) {
    DragQueryFile(drop, 0, TheFilename, MAX_PATH);
    command(CmdOpenFile);
}

init_ui_input(HWND hwnd) {
    cursor_resize_col = LoadCursor(0, IDC_SIZEWE);
    cursor_arrow = LoadCursor(0, IDC_ARROW);
    cursor_scrolling = LoadCursor(0, IDC_SIZEALL);
    open_dlg.hwndOwner = hwnd;
    
    EditBox = CreateWindowEx(0, TEXT("EDIT"), TEXT(""),
        WS_CHILD | ES_AUTOHSCROLL | ES_AUTOVSCROLL
        | ES_MULTILINE | ES_WANTRETURN, 0, 0, 0, 0,
        hwnd, 0, GetModuleHandle(0), 0);
    SetWindowFont(EditBox, font_cell, 0);
    SetWindowSubclass(EditBox, EditProc, 0, 0);
}
