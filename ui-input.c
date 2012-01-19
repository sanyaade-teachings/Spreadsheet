#define MOUSE_SCROLL_STEP_X 75
#define MOUSE_SCROLL_STEP_Y 25

enum {
    MOUSE_MODE_NORMAL,
    MOUSE_MODE_RESIZING,
    MOUSE_MODE_SCROLLING
};


HCURSOR     cursor_resize_col;
HCURSOR     cursor_arrow;
HCURSOR     cursor_scrolling;

HWND        dlg;

FINDREPLACEA find_dialog_template = {
	sizeof find_dialog_template, 0, 0,
	FR_DOWN|FR_DIALOGTERM|FR_HIDEMATCHCASE|FR_HIDEWHOLEWORD,
	0, 0, FIND_TEXT_MAX, 0, 0, 0, 0 };

OPENFILENAME open_dialog_template = {
    sizeof open_dialog_template, 0, 0,
    L"All Spreadsheets (*.csv;*.txt)\0*.csv;*.txt\0"
        L"Comma Seperated Values (*.csv)\0*.csv\0"
        L"Text File (*.txt)\0*.txt\0"
        L"All Files (*.*)\0*.*\0",
    0, 0, 0, 0, MAX_PATH, 0, 0, 0, 0,
    OFN_FILEMUSTEXIST | OFN_OVERWRITEPROMPT, };

exit_mouse_mode(TableUI *tui);

#define IsShiftDown() (GetKeyState(VK_SHIFT) < 0)
#define IsCtrlDown() (GetKeyState(VK_CONTROL) < 0)

is_editing(TableUI *tui) {
    return GetWindowStyle(tui->edit) & WS_VISIBLE;
}

cancel_edit(TableUI *tui) {
    SetFocus(tui->window);
    ShowWindow(tui->edit, SW_HIDE);
}

end_edit(TableUI *tui) {
    if (is_editing(tui)) {
        char buf[65536];           /* An EDIT control's limit */
        int len = GetWindowTextLength(tui->edit);
        GetWindowTextA (tui->edit, buf, len + 1);
        set_cell(tui->table, tui->cur_row, tui->cur_col, buf, len);
        cancel_edit(tui);
        redraw_rows(tui, tui->cur_row, tui->cur_row);
    }
}

start_edit(TableUI *tui, int edit_existing) {
    RECT rt = get_cell_rect(tui, tui->cur_row, tui->cur_col), rt_cont = rt;    
    
    if (is_editing(tui)) end_edit(tui);
    
    if (edit_existing) {
        Cell cell = try_cell(tui->table, tui->cur_row, tui->cur_col);
        SetWindowTextA(tui->edit, cell.str);
        
        /* Resize Edit to fit content; at least as big as the cell */
        DrawTextA(tui->bitmap, cell.str, cell.len, &rt_cont,
            DT_NOPREFIX | DT_RIGHT | DT_CALCRECT | DT_EDITCONTROL);
        UnionRect(&rt, &rt, &rt_cont);
    } else
        SetWindowText(tui->edit, L"");
    
    MoveWindow(tui->edit, rt.left, rt.top,
        rt.right - rt.left, rt.bottom - rt.top, 0);
    ShowWindow(tui->edit, SW_NORMAL);
    SetFocus(tui->edit);
}

LRESULT CALLBACK
EditProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, UINT_PTR id, DWORD_PTR data) {
    TableUI *tui = (void*)data;
    if (msg == WM_CHAR)
        switch (wparam) {
        case VK_RETURN:
            if (IsCtrlDown()) break;
            command(tui, CmdReturn);
            command(tui, CmdCommitEdit);
            return 0;
        case VK_TAB:
            if (IsCtrlDown()) break;
            command(tui, CmdTab);
            command(tui, CmdCommitEdit);
            return 0;
        case VK_ESCAPE:
            command(tui, CmdCancelEdit);
            return 0;
        }
    return DefSubclassProc(hwnd, msg, wparam, lparam);
}

wm_char(TableUI *tui, unsigned wparam) {

    switch (wparam) {
    
    case 'F' - 'A' + 1:
        if (!dlg) {
            tui->find_dialog.hwndOwner = tui->window;
            dlg = FindTextA(&tui->find_dialog);
        }
        break;
    
    case 'L' - 'A' + 1:                         /* Delete Row */
        if (IsShiftDown()) command(tui, CmdDeleteRow);
        else command(tui, CmdClearRow);
        break;
        
    case 'N' - 'A' + 1:                           /* New File */
        command(tui, CmdClearFile);
        break;
        
    case 'O' - 'A' + 1:                          /* Open file */
        if (GetOpenFileName(&tui->open_dialog))        
            command(tui, CmdOpenFile);
        break;
    
    case 'S' - 'A' + 1:                          /* Save File */
        if (tui->filename[0] || GetSaveFileName(&tui->open_dialog))
            command(tui, CmdSaveFile);
        break;
    
    case 'C' - 'A' + 1:                               /* Copy */
        command(tui, CmdCopy);
        break;
    
    case 'X' - 'A' + 1:                                /* Cut */
        command(tui, IsShiftDown()? CmdCutDelete: CmdCutClear);
        break;
    
    case 'V' - 'A' + 1:                              /* Paste */
        command(tui, CmdPaste);
        break;
        
    case VK_RETURN:                                  /* Enter */
        command(tui, IsShiftDown()? CmdUnReturn: CmdReturn);
        break;
        
    case VK_TAB:                                       /* Tab */
        command(tui, IsShiftDown()? CmdUnTab: CmdTab);
        break;
    
    default:                                /* Auto-edit cell */
        /* Don't drop the char; forward it to the editor */
        command(tui, CmdEditCellClear);
        SendMessage(tui->edit, WM_CHAR, wparam, 0); 
        break;
    }
    return 1;
}

wm_keydown(TableUI *tui, unsigned wparam) {

    switch (wparam) {
    
    case VK_ESCAPE:
        if (exit_mouse_mode(tui))
            break;
        break;
    
    case VK_UP:
        if (IsCtrlDown())
            command(tui, CmdScrollUp);
        else {
            command(tui, IsShiftDown()? CmdSetAnchor: CmdClearAnchor);
            command(tui, CmdMoveUp);
        }
        break;
    case VK_DOWN:
        if (IsCtrlDown())
            command(tui, CmdScrollDown);
        else  {
            command(tui, IsShiftDown()? CmdSetAnchor: CmdClearAnchor);
            command(tui, CmdMoveDown);
        }
        break;
    case VK_LEFT:
        if (IsCtrlDown())
            command(tui, CmdScrollLeft);
        else {
            command(tui, IsShiftDown()? CmdSetAnchor: CmdClearAnchor);
            command(tui, CmdMoveLeft);
        }
        break;
    case VK_RIGHT:
        if (IsCtrlDown())
            command(tui, CmdScrollRight);
        else {
            command(tui, IsShiftDown()? CmdSetAnchor: CmdClearAnchor);
            command(tui, CmdMoveRight);
        }
        break;
    
    case VK_F2:
        command(tui, CmdEditCell);
        break;
    
    case VK_F3:
        command(tui, IsCtrlDown()? CmdFindColumn: CmdFindRow);
        break;
    
    case VK_HOME:
        command(tui, IsShiftDown()? CmdSetAnchor: CmdClearAnchor);
        command(tui, IsCtrlDown()? CmdHomeCol: CmdHomeRow);    
        break;
        
    case VK_END:
        command(tui, IsShiftDown()? CmdSetAnchor: CmdClearAnchor);
        command(tui, IsCtrlDown()? CmdEndCol: CmdEndRow);    
        break;
    
    case VK_DELETE:
        command(tui, IsShiftDown()? CmdDeleteCell: CmdClearCell) ;
        break;
    
    case VK_OEM_1: /* Semicolon */
        if (IsCtrlDown()) {               /* Insert Date/Time */
            command(tui, IsShiftDown()? CmdInsertDateTime: CmdInsertDate);
            break;
        }
        return 0;

    case VK_OEM_PERIOD: /* . */
        if (IsCtrlDown()) {
            command(tui, IsShiftDown()? CmdInsertCell: CmdInsertRow);
            break;
        }
        return 0;
    }
    return 1;
}

wm_setfocus(TableUI *tui) {
    if (is_editing(tui)) SetFocus(tui->edit);
}

wm_find(TableUI *tui, FINDREPLACE *find) {
    if (find->Flags & FR_FINDNEXT)
        command(tui, CmdFindRow);
    else if (find->Flags & FR_DIALOGTERM)
        dlg = 0;
}

void CALLBACK ScrollTimerProc(HWND hwnd, UINT msg, UINT_PTR id, DWORD time) {
    DWORD pos = GetMessagePos();
    TableUI *tui = get_tui(hwnd);
    POINT pt;
    pt.y = GET_Y_LPARAM(pos);
    pt.x = GET_X_LPARAM(pos);
    ScreenToClient(hwnd, &pt);
    scroll(tui,
        (pt.y - tui->mouse_scroll_anchor.y) / MOUSE_SCROLL_STEP_Y,
        (pt.x - tui->mouse_scroll_anchor.x) / MOUSE_SCROLL_STEP_X);
}

exit_mouse_mode(TableUI *tui) {
    unsigned old_mode = tui->mouse_mode;
    
    switch (tui->mouse_mode) {
    
    case MOUSE_MODE_SCROLLING:
        KillTimer(tui->window, 1);
        ReleaseCapture();
        break;
    
    case MOUSE_MODE_RESIZING:
        SetCursor(cursor_arrow);
        ReleaseCapture();
        break;
    }
    tui->mouse_mode = 0;
    return old_mode;
}

enter_mouse_mode(TableUI *tui, unsigned mode, unsigned x, unsigned y) {
    if (!exit_mouse_mode(tui))/* Only changed modes if coming from normal */
        switch (tui->mouse_mode = mode) {
        
        case MOUSE_MODE_SCROLLING:
            tui->mouse_scroll_anchor.x = x;
            tui->mouse_scroll_anchor.y = y;
            SetCursor(cursor_scrolling);
            SetTimer(tui->window, 1, 100, ScrollTimerProc);
            SetCapture(tui->window);
            break;
        
        case MOUSE_MODE_RESIZING:
            tui->resizing_col = y;
            SetCapture(tui->window);
            break;
        
        }
}

wm_mbuttondown(TableUI *tui, unsigned x, unsigned y) {
    enter_mouse_mode(tui, MOUSE_MODE_SCROLLING, x, y);
}
wm_mbuttonup(TableUI *tui, unsigned x, unsigned y) {
    exit_mouse_mode(tui);
}

wm_lbuttondown(TableUI *tui, unsigned x, unsigned y) {
    unsigned row, col, is_resize;
    
    exit_mouse_mode(tui);
    get_cell_under(tui, x, y, &row, &col, &is_resize);
    if (is_resize)
        enter_mouse_mode(tui, MOUSE_MODE_RESIZING, 0, col);
    else {
        command(tui, IsShiftDown()? CmdSetAnchor: CmdClearAnchor);
        jump_cursor(tui, row, col);
    }
}

wm_lbuttonup(TableUI *tui, unsigned x, unsigned y) {
    exit_mouse_mode(tui);
}

wm_mousemove(TableUI *tui, unsigned x, unsigned y) {
    if (tui->mouse_mode == MOUSE_MODE_RESIZING) {
        resize_col(tui, tui->resizing_col, x - get_cell_x(tui, tui->resizing_col + 1));
        redraw_rows(tui, 0, -1);
    } else {
        unsigned row, col, is_resize;
        get_cell_under(tui, x, y, &row, &col, &is_resize);
        if (is_resize)
            SetCursor(cursor_resize_col);
        else if (GetCursor() == cursor_resize_col)
            SetCursor(cursor_arrow);
    }
}

wm_lbuttondblclk(TableUI *tui, unsigned x, unsigned y) {
    unsigned row, col, is_resize;
    
    exit_mouse_mode(tui);
    
    get_cell_under(tui, x, y, &row, &col, &is_resize);
    if (is_resize)
        auto_resize_col(tui, col);
    else {
        command(tui, IsShiftDown()? CmdSetAnchor: CmdClearAnchor);
        jump_cursor(tui, row, col);
        command(tui, CmdEditCell);
        start_edit(tui, 1);
    }
}

wm_mousewheel(TableUI *tui, int delta) {
    if (tui->mouse_mode == MOUSE_MODE_NORMAL)
        scroll(tui, delta / -WHEEL_DELTA, 0);
}

wm_dropfiles(TableUI *tui, HDROP drop) {
    DragQueryFile(drop, 0, tui->filename, MAX_PATH);
    command(tui, CmdOpenFile);
}

init_ui_input(TableUI *tui) {
    cursor_resize_col = LoadCursor(0, IDC_SIZEWE);
    cursor_arrow = LoadCursor(0, IDC_ARROW);
    cursor_scrolling = LoadCursor(0, IDC_SIZEALL);
    
    tui->find_dialog = find_dialog_template;
    tui->find_dialog.hwndOwner = tui->window;
    tui->find_dialog.lpstrFindWhat = tui->find_text;
    tui->open_dialog = open_dialog_template;
    tui->open_dialog.hwndOwner = tui->window;
    tui->open_dialog.lpstrFile = tui->filename;
    
    tui->edit = CreateWindowEx(0, TEXT("EDIT"), TEXT(""),
        WS_CHILD | ES_AUTOHSCROLL | ES_AUTOVSCROLL
        | ES_MULTILINE | ES_WANTRETURN, 0, 0, 0, 0,
        tui->window, 0, GetModuleHandle(0), 0);
    SetWindowFont(tui->edit, font_cell, 0);
    SetWindowSubclass(tui->edit, EditProc, 0, (DWORD_PTR)tui);
}
