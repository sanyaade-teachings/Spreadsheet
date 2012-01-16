Table       TheTable;

unsigned    CurRow, CurCol;
unsigned    AnchorRow, AnchorCol;
#define     SelStartRow min(CurRow, AnchorRow)
#define     SelStartCol min(CurCol, AnchorCol)
#define     SelEndRow   (max(CurRow, AnchorRow) + 1)
#define     SelEndCol   (max(CurCol, AnchorCol) + 1)
#define     SelRows     (SelEndRow - SelStartRow)
#define     SelCols     (SelEndCol - SelStartCol)
BOOL        is_selecting;

is_editing();
cancel_edit();
end_edit() ;
start_edit(int edit_existing);
snap_to_cursor();
reset_column_sizes();

unsigned clamp(unsigned a, unsigned b, unsigned c) {
    return min(max(a, b), c);
}

set_anchor() {                        /* Set Selection Anchor */
    if (!is_selecting) {
        AnchorRow = CurRow;
        AnchorCol = CurCol;
        is_selecting = 1;
    }
}

clear_anchor() {                    /* Clear Selection Anchor */
    if (is_selecting) {
        is_selecting = 0;
        redraw_rows(0, -1);
    }
}

jump_cursor(unsigned row, unsigned col) {
    unsigned ocol = CurCol, orow = CurRow;
    
    end_edit();
    CurRow = row;
    CurCol = col;
    snap_to_cursor();
    
    if (is_selecting)
        if (CurCol != ocol || 1 < abs(orow - CurRow)) {
            /* Moving up or down one only changes one of two rows */
            /* Any left or right move changes the width of every row */
            redraw_rows(min(orow, SelStartRow), max(orow, SelEndRow+1));
        }
    
    /* Clear old cursor and draw new one */
    redraw_rows(min(orow, CurRow), max(orow,CurRow));
}

move_cursor(int row, int col) {
    jump_cursor(max(0, (int)CurRow + row), max(0, (int)CurCol + col));
}

copy_to_clipboard() {
    if (OpenClipboard(TheWindow)) {
        HANDLE  handle;
        char    *name = tempnam("", "csv");
        unsigned len;
        FILE    *tmpf = fopen(name, "wb+");
        
        EmptyClipboard();
        
        if (is_selecting)
            write_csv_cells(tmpf, &TheTable, SelStartRow, SelEndRow,
                SelStartCol, SelEndCol);
        else
            write_csv_cells(tmpf, &TheTable, CurRow, CurRow + 1,
                CurCol, CurCol + 1);            

        len = ftell(tmpf);
        if ( handle = GlobalAlloc(GMEM_MOVEABLE, len + 1) ) {
            char *text = GlobalLock(handle);
            rewind(tmpf);
            fread(text, 1, len, tmpf);
            GlobalUnlock(handle);
            SetClipboardData(CF_TEXT, handle);
        }
        fclose(tmpf);
        unlink(name);
        CloseClipboard();
    }
}

paste_clipboard() {
    if (OpenClipboard(TheWindow)) {
        HANDLE handle = GetClipboardData(CF_TEXT);
        char *text = GlobalLock(handle);
        unsigned len = GlobalSize(handle);
        read_csv(&TheTable, CurRow, CurCol, text, text + len);
        GlobalUnlock(handle);
        CloseHandle(handle);
        CloseClipboard();
        snap_to_cursor();
        redraw_rows(CurRow, -1);
    }
}

clear_selected_cells() {
    unsigned r;
    if (is_selecting) {
        for (r = SelStartRow; r < SelEndRow; r++)
            clear_cells(&TheTable, r, SelStartCol, SelCols);
        redraw_rows(SelStartRow, SelEndRow - 1);
    } else {
        clear_cells(&TheTable, CurRow, CurCol, 1);
        redraw_rows(CurRow, CurRow);
    }
}

delete_selected_cells() {
    unsigned r;
    if (is_selecting) {
        for (r = SelStartRow; r < SelEndRow; r++)
            delete_cells(&TheTable, r, SelStartCol, SelCols);
        redraw_rows(SelStartRow, SelEndRow - 1);
    } else {
        delete_cells(&TheTable, CurRow, CurCol, 1);
        redraw_rows(CurRow, CurRow);
    }
}

clear_selected_rows() {
    if (is_selecting) {
        clear_rows(&TheTable, SelStartRow, SelRows);
        redraw_rows(SelStartRow, SelEndRow - 1);
    } else {
        clear_rows(&TheTable, CurRow, 1);
        redraw_rows(CurRow, CurRow);
    }
}

delete_selected_rows() {
    if (is_selecting) {
        delete_rows(&TheTable, SelStartRow, SelRows);
        redraw_rows(SelStartRow, -1);
    } else {
        delete_rows(&TheTable, CurRow, 1);
        redraw_rows(CurRow, -1);
    }
}

open_csv(TCHAR *fn) {
    FILE *f = fn? _wfopen(fn, L"rb"): 0;
    char *data;
    unsigned len;
    
    if (!fn || !f) return 0;
    
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    rewind(f);
    fread(data = malloc(len), 1, len, f);
    fclose(f);
    
    read_csv(&TheTable, 0, 0, data, data + len);
    free(data);
    
    jump_cursor(0, 0);
    redraw_rows(0, -1);
    return 1;
}

clear_file() {
    TheFilename[0] = 0;
    reset_column_sizes();
    delete_table(&TheTable);
    redraw_rows(0, -1);
    is_selecting = 0;
}

clear_and_open(TCHAR *fn) {
    reset_column_sizes();
    delete_table(&TheTable); /* Do not use clear_file(); it clears the filename */
    if (!open_csv(TheFilename))
        MessageBox(TheWindow, L"Could not open the file", L"Error", MB_OK);
    is_selecting = 0;
    redraw_rows(0, -1);
}

save_csv(TCHAR *fn) {
    FILE *f = _wfopen(fn, L"wb");
    if (f) {
        write_csv(f, TheTable);
        fclose(f);
    } else
        MessageBox(TheWindow, L"Could not save the file", L"Error", MB_OK);
    return !!f;
}
