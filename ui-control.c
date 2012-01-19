#define     SelStartRow min(tui->cur_row, tui->anchor_row)
#define     SelStartCol min(tui->cur_col, tui->anchor_col)
#define     SelEndRow   (max(tui->cur_row, tui->anchor_row) + 1)
#define     SelEndCol   (max(tui->cur_col, tui->anchor_col) + 1)
#define     SelRows     (SelEndRow - SelStartRow)
#define     SelCols     (SelEndCol - SelStartCol)

is_editing(TableUI *tui);
cancel_edit(TableUI *tui);
end_edit(TableUI *tui) ;
start_edit(TableUI *tui, int edit_existing);
snap_to_cursor(TableUI *tui);
reset_col_sizes(TableUI *tui);
redraw_rows(TableUI *tui, unsigned lo, unsigned hi);

unsigned clamp(unsigned a, unsigned b, unsigned c) {
    return min(max(a, b), c);
}

set_anchor(TableUI *tui) {            /* Set Selection Anchor */
    if (!tui->is_selecting) {
        tui->anchor_row = tui->cur_row;
        tui->anchor_col = tui->cur_col;
        tui->is_selecting = 1;
    }
}

clear_anchor(TableUI *tui) {        /* Clear Selection Anchor */
    if (tui->is_selecting) {
        tui->is_selecting = 0;
        redraw_rows(tui, 0, -1);
    }
}

jump_cursor(TableUI *tui, unsigned row, unsigned col) {
    unsigned ocol = tui->cur_col, orow = tui->cur_row;
    
    end_edit(tui);
    tui->cur_row = row;
    tui->cur_col = col;
    snap_to_cursor(tui);
    
    if (tui->is_selecting)
        if (tui->cur_col != ocol || 1 < abs(orow - tui->cur_row)) {
            /* Moving up or down one only changes one of two rows */
            /* Any left or right move changes the width of every row */
            redraw_rows(tui, min(orow, SelStartRow), max(orow, SelEndRow+1));
        }
    
    /* Clear old cursor and draw new one */
    redraw_rows(tui, min(orow, tui->cur_row), max(orow,tui->cur_row));
}

move_cursor(TableUI *tui, int row, int col) {
    jump_cursor(tui, max(0, (int)tui->cur_row + row), max(0, (int)tui->cur_col + col));
}

copy_to_clipboard(TableUI *tui) {
    if (OpenClipboard(tui->window)) {
        HANDLE  handle;
        char    *name = tempnam("", "csv");
        unsigned len;
        FILE    *tmpf = fopen(name, "wb+");
        
        EmptyClipboard();
        
        if (tui->is_selecting)
            write_csv_cells(tmpf, tui->table, SelStartRow, SelEndRow,
                SelStartCol, SelEndCol);
        else
            write_csv_cells(tmpf, tui->table, tui->cur_row, tui->cur_row + 1,
                tui->cur_col, tui->cur_col + 1);            

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

paste_clipboard(TableUI *tui) {
    if (OpenClipboard(tui->window)) {
        HANDLE handle = GetClipboardData(CF_TEXT);
        char *text = GlobalLock(handle);
        unsigned len = GlobalSize(handle);
        read_csv(tui->table, tui->cur_row, tui->cur_col, text, text + len);
        GlobalUnlock(handle);
        CloseHandle(handle);
        CloseClipboard();
        snap_to_cursor(tui);
        redraw_rows(tui, tui->cur_row, -1);
    }
}

clear_selected_cells(TableUI *tui) {
    unsigned r;
    if (tui->is_selecting) {
        for (r = SelStartRow; r < SelEndRow; r++)
            clear_cells(tui->table, r, SelStartCol, SelCols);
        redraw_rows(tui, SelStartRow, SelEndRow - 1);
    } else {
        clear_cells(tui->table, tui->cur_row, tui->cur_col, 1);
        redraw_rows(tui, tui->cur_row, tui->cur_row);
    }
}

delete_selected_cells(TableUI *tui) {
    unsigned r;
    if (tui->is_selecting) {
        for (r = SelStartRow; r < SelEndRow; r++)
            delete_cells(tui->table, r, SelStartCol, SelCols);
        redraw_rows(tui, SelStartRow, SelEndRow - 1);
    } else {
        delete_cells(tui->table, tui->cur_row, tui->cur_col, 1);
        redraw_rows(tui, tui->cur_row, tui->cur_row);
    }
}

clear_selected_rows(TableUI *tui) {
    if (tui->is_selecting) {
        clear_rows(tui->table, SelStartRow, SelRows);
        redraw_rows(tui, SelStartRow, SelEndRow - 1);
    } else {
        clear_rows(tui->table, tui->cur_row, 1);
        redraw_rows(tui, tui->cur_row, tui->cur_row);
    }
}

delete_selected_rows(TableUI *tui) {
    if (tui->is_selecting) {
        delete_rows(tui->table, SelStartRow, SelRows);
        redraw_rows(tui, SelStartRow, -1);
    } else {
        delete_rows(tui->table, tui->cur_row, 1);
        redraw_rows(tui, tui->cur_row, -1);
    }
}

open_csv(TableUI *tui, TCHAR *fn) {
    FILE *f = fn? _wfopen(fn, L"rb"): 0;
    char *data;
    unsigned len;
    
    if (!fn || !f) return 0;
    
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    rewind(f);
    fread(data = malloc(len), 1, len, f);
    fclose(f);
    
    read_csv(tui->table, 0, 0, data, data + len);
    free(data);
    
    jump_cursor(tui, 0, 0);
    redraw_rows(tui, 0, -1);
    return 1;
}

clear_file(TableUI *tui) {
    tui->filename[0] = 0;
    reset_col_sizes(tui);
    delete_table(tui->table);
    redraw_rows(tui, 0, -1);
    tui->is_selecting = 0;
}

clear_and_open(TableUI *tui, TCHAR *fn) {
    reset_col_sizes(tui);
    delete_table(tui->table); /* Do not use clear_file(); it clears the filename */
    if (!open_csv(tui, tui->filename))
        MessageBox(tui->window, L"Could not open the file", L"Error", MB_OK);
    tui->is_selecting = 0;
    redraw_rows(tui, 0, -1);
}

save_csv(TableUI *tui, TCHAR *fn) {
    FILE *f = _wfopen(fn, L"wb");
    if (f) {
        write_csv(f, *tui->table);
        fclose(f);
    } else
        MessageBox(tui->window, L"Could not save the file", L"Error", MB_OK);
    return !!f;
}
