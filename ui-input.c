OPENFILENAME    open_dlg = {
    sizeof open_dlg, 0, 0,
    L"Comma Seperated Values (*.csv)\0*.csv\0"
        L"Text File (*.txt)\0*.txt\0",
    0, 0, 0, TheFilename, MAX_PATH, 0, 0, 0, 0,
    OFN_FILEMUSTEXIST | OFN_OVERWRITEPROMPT, };

#define IsShiftDown() (GetKeyState(VK_SHIFT) < 0)
#define IsCtrlDown() (GetKeyState(VK_CONTROL) < 0)

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
            end_edit();
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

wm_char(HWND hwnd, unsigned wparam) {
    switch (wparam) {
    
    case 'L' - 'A' + 1:                         /* Delete Row */
        delete_selected_rows(IsShiftDown());
        break;
        
    case 'N' - 'A' + 1:                           /* New File */
        clear_file();
        break;
        
    case 'O' - 'A' + 1:                          /* Open file */
        if (GetOpenFileName(&open_dlg))
            clear_and_open(TheFilename);
        break;
    
    case 'S' - 'A' + 1:                          /* Save File */
        if (TheFilename[0] || GetSaveFileName(&open_dlg))
            if (!save_csv(TheFilename))
                MessageBox(hwnd, L"Could not save the file", L"Error", MB_OK);
        break;
    
    case 'C' - 'A' + 1:                               /* Copy */
        copy_to_clipboard();
        break;
    
    case 'X' - 'A' + 1:                                /* Cut */
        copy_to_clipboard();
        delete_selected_cols(IsShiftDown());
        break;
    
    case 'V' - 'A' + 1:                              /* Paste */
        clear_anchor();
        paste_clipboard();
        break;
        
    case VK_RETURN:                                  /* Enter */
        clear_anchor();
        move_cursor(IsShiftDown()? -1: 1, 0);
        break;
        
    case VK_TAB:                                       /* Tab */
        clear_anchor();
        move_cursor(0, IsShiftDown()? -1: 1);
        break;
    
    default:                                /* Auto-edit cell */
        /* Don't drop the char; forward it to the editor */
        start_edit(0);
        SendMessage(EditBox, WM_CHAR, wparam, 0); 
        break;
    }
    return 1;
}

wm_keydown(HWND hwnd, unsigned wparam) {
    switch (wparam) {
    
    case VK_UP:
        if (IsCtrlDown()) scroll(-1, 0);
        else {
            if (IsShiftDown()) set_anchor(); else clear_anchor();
            move_cursor(-1, 0);
        }
        break;
    case VK_DOWN:
        if (IsCtrlDown()) scroll(1, 0);
        else  {
            if (IsShiftDown()) set_anchor(); else clear_anchor();
            move_cursor(1, 0);
        }
        break;
    case VK_LEFT:
        if (IsCtrlDown()) scroll(0, -1);
        else {
            if (IsShiftDown()) set_anchor(); else clear_anchor();
            move_cursor(0, -1);
        }
        break;
    case VK_RIGHT:
        if (IsCtrlDown()) scroll(0, 1);
        else {
            if (IsShiftDown()) set_anchor(); else clear_anchor();
            move_cursor(0, 1);
        }
        break;
    
    case VK_F2: start_edit(1); break;
    
    case VK_HOME:
        if (IsShiftDown()) set_anchor();
        if (IsCtrlDown())
            jump_cursor(0, CurCol);
        else
            jump_cursor(CurRow, 0);
        break;
        
    case VK_END:
        if (IsShiftDown()) set_anchor(); else clear_anchor();
        if (IsCtrlDown())
            jump_cursor(row_count(&TheTable) - 1, CurCol);
        else
            jump_cursor(CurRow, col_count(&TheTable, CurRow) - 1);
        break;
    
    case VK_DELETE:
        delete_selected_cols(IsShiftDown());
        break;
    
    case VK_OEM_1: /* Semicolon */
        if (IsCtrlDown()) {               /* Insert Date/Time */
            char timestr[32];
            SYSTEMTIME time;
            GetLocalTime(&time);
            if (IsShiftDown())
                sprintf(timestr, "%04d-%02d-%02d %02d:%02d:%02d",
                    time.wYear, time.wMonth, time.wDay,
                    time.wHour, time.wMinute, time.wSecond);
            else
                sprintf(timestr, "%04d-%02d-%02d",
                    time.wYear, time.wMonth, time.wDay);
            set_cell(&TheTable, CurRow, CurCol, timestr, strlen(timestr));
            redraw_rows(CurRow, CurRow);
            break;
        }
        return 0;

    case VK_OEM_PERIOD: /* . */
        if (IsCtrlDown())
            if (IsShiftDown()) {               /* Insert Cell */
                insert_cell(&TheTable, CurRow, CurCol);
                redraw_rows(CurRow, CurRow);
                return 0;
            } else {                            /* Insert Row */
                insert_row(&TheTable, CurRow);
                redraw_rows(CurRow, -1);
                return 0;
            }
        break;
    }
    return 1;
}

wm_lbuttondown(HWND hwnd, unsigned x, unsigned y) {
    if (IsShiftDown()) set_anchor(); else clear_anchor();
    jump_cursor(y / CellHeight + FirstRow, x / CellWidth + FirstCol);
}

wm_lbuttondblclk(HWND hwnd, unsigned x, unsigned y) {
    if (IsShiftDown()) set_anchor(); else clear_anchor();
    jump_cursor(y / CellHeight + FirstRow, x / CellWidth + FirstCol);
    start_edit(1);
}

wm_dropfiles(HWND hwnd, HDROP drop) {
    DragQueryFile(drop, 0, TheFilename, MAX_PATH);
    clear_and_open(TheFilename);
}

init_ui_input(HWND hwnd) {
    open_dlg.hwndOwner = hwnd;
    
    EditBox = CreateWindowEx(0, TEXT("EDIT"), TEXT(""),
        WS_CHILD | ES_RIGHT | ES_AUTOHSCROLL | ES_AUTOVSCROLL
        | ES_MULTILINE | ES_WANTRETURN, 0, 0, 0, 0,
        hwnd, 0, GetModuleHandle(0), 0);
    SetWindowFont(EditBox, font_cell, 0);
    SetWindowSubclass(EditBox, EditProc, 0, 0);
}
