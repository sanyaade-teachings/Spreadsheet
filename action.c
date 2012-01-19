enum {
    CmdClearFile,
    CmdOpenFile,
    CmdSaveFile,
    
    CmdDeleteRow,
    CmdClearRow,
    CmdDeleteCell,
    CmdClearCell,
    
    CmdSetAnchor,
    CmdClearAnchor,
    
    CmdCopy,
    CmdCutClear,
    CmdCutDelete,
    CmdPaste,
    
    CmdReturn,
    CmdTab,
    CmdUnReturn,
    CmdUnTab,
    
    CmdEditCell,
    CmdEditCellClear,
    CmdCommitEdit,
    CmdCancelEdit,
    
    CmdMoveLeft,
    CmdMoveRight,
    CmdMoveUp,
    CmdMoveDown,
    
    CmdScrollLeft,
    CmdScrollRight,
    CmdScrollUp,
    CmdScrollDown,
    
    CmdHomeCol,
    CmdHomeRow,
    CmdEndCol,
    CmdEndRow,
    
    CmdInsertDate,
    CmdInsertDateTime,
    
    CmdInsertCell,
    CmdInsertRow,
    
    CmdFindColumn,
    CmdFindRow,
    
};

scroll(TableUI *tui, int row, int col);
auto_resize_col(TableUI *tui, unsigned col);

insert_datetime(TableUI *tui, int include_time) {
    char timestr[32];
    SYSTEMTIME time;
    GetLocalTime(&time);
    if (include_time)
        sprintf(timestr, "%04d-%02d-%02d %02d:%02d:%02d",
            time.wYear, time.wMonth, time.wDay,
            time.wHour, time.wMinute, time.wSecond);
    else
        sprintf(timestr, "%04d-%02d-%02d",
            time.wYear, time.wMonth, time.wDay);
    set_cell(tui->table, tui->cur_row, tui->cur_col, timestr, strlen(timestr));
    redraw_rows(tui, tui->cur_row, tui->cur_row);
}

cell_has_text(Table *table, unsigned row, unsigned col, char *text) {
    Cell cell = try_cell(table, row, col);
    return !!strstr(cell.str, text);
}

find_cell_text_col(TableUI *tui, unsigned row, unsigned col, char *text) {
    Table *table = tui->table;
    unsigned r,c;
    
    /* Find in current column after current row */
    for (c = col, r = row + 1; r < row_count(table); r++)
        if (cell_has_text(table, r, c, text)) goto found;
    
    /* Find in columns after the current */
    for (c = col + 1; c < max_col_count(table); c++)
        for (r = 0; r < row_count(table); r++)
            if (cell_has_text(table, r, c, text)) goto found;
    
    /* Wrap around up to columns before current */
    for (c = 0; c <= col; c++)
        for (r = 0; r < row_count(table); r++)
            if (cell_has_text(table, r, c, text)) goto found;
    
    return 0;
    
found:
    jump_cursor(tui, r, c);
    return 1;
}

find_cell_text_row(TableUI *tui, unsigned row, unsigned col, char *text) {
    Table *table = tui->table;
    unsigned r,c;
    
    /* Find in current row after current column */
    for (r = row, c = col + 1; c < col_count(table, r); c++)
        if (cell_has_text(table, r, c, text)) goto found;
    
    /* Find in rows after current */
    for (r = row + 1; r < row_count(table); r++)
        for (c = 0; c < col_count(table, r); c++)
            if (cell_has_text(table, r, c, text)) goto found;
    
    /* Wrap around and find in rows before current */
    for (r = 0; r < row; r++)
        for (c = 0; c < col_count(table, r); c++)
            if (cell_has_text(table, r, c, text)) goto found;
    
    return 0;
    
found:
    jump_cursor(tui, r, c);
    return 1;
}

command(TableUI *tui, int cmd) {

    switch (cmd) {
    
    case CmdClearFile: clear_file(tui); break;
    case CmdOpenFile:
        clear_and_open(tui, tui->filename);
        if (row_count(tui->table) < MAX_ROWS_FOR_FIT
         && max_col_count(tui->table) < MAX_COLS_FOR_FIT) {
            unsigned i;
            for (i = 0; i < max_col_count(tui->table); i++)
                auto_resize_col(tui, i);
        }
        break;
    case CmdSaveFile: save_csv(tui, tui->filename); break;
    
    case CmdSetAnchor: set_anchor(tui); break;
    case CmdClearAnchor: clear_anchor(tui); break;
    
    case CmdClearRow: clear_selected_rows(tui); break;
    case CmdDeleteRow: delete_selected_rows(tui); break;
    case CmdDeleteCell: delete_selected_cells(tui); break;
    case CmdClearCell: clear_selected_cells(tui); break;
    
    case CmdCopy: copy_to_clipboard(tui); break;
    case CmdCutClear: copy_to_clipboard(tui); clear_selected_cells(tui); break;
    case CmdCutDelete: copy_to_clipboard(tui); delete_selected_cells(tui); break;
    case CmdPaste: clear_anchor(tui); paste_clipboard(tui); break;
    
    case CmdReturn: clear_anchor(tui); move_cursor(tui, 1, 0); break;
    case CmdTab: clear_anchor(tui); move_cursor(tui, 0, 1); break;
    case CmdUnReturn: clear_anchor(tui); move_cursor(tui, -1, 0); break;
    case CmdUnTab: clear_anchor(tui); move_cursor(tui, 0, -1); break;
    
    case CmdEditCell: start_edit(tui, 1); break;
    case CmdEditCellClear: start_edit(tui, 0); break;
    case CmdCommitEdit: end_edit(tui); break;
    case CmdCancelEdit: cancel_edit(tui); break;
    
    case CmdMoveUp: move_cursor(tui, -1, 0); break;
    case CmdMoveDown: move_cursor(tui, 1, 0); break;
    case CmdMoveLeft: move_cursor(tui, 0, -1); break;
    case CmdMoveRight: move_cursor(tui, 0, 1); break;
    
    case CmdScrollUp: scroll(tui, -1, 0); break;
    case CmdScrollDown: scroll(tui, 1, 0); break;
    case CmdScrollLeft: scroll(tui, 0, -1); break;
    case CmdScrollRight: scroll(tui, 0, 1); break;
    
    case CmdHomeCol: jump_cursor(tui, 0, tui->cur_col); break;
    case CmdHomeRow: jump_cursor(tui, tui->cur_row, 0); break;
    case CmdEndCol: jump_cursor(tui, row_count(tui->table) - 1, tui->cur_col); break;
    case CmdEndRow: jump_cursor(tui, tui->cur_row, col_count(tui->table, tui->cur_row) - 1); break;
    
    case CmdInsertDate: insert_datetime(tui, 0); break;
    case CmdInsertDateTime: insert_datetime(tui, 1); break;
    
    case CmdInsertCell:
        insert_cells(tui->table, tui->cur_row, tui->cur_col, 1);
        redraw_rows(tui, tui->cur_row, tui->cur_row);
        break;
    case CmdInsertRow:
        insert_rows(tui->table, tui->cur_row, 1);
        redraw_rows(tui, tui->cur_row, -1);
        break;
    
    case CmdFindColumn:
        find_cell_text_col(tui, tui->cur_row, tui->cur_col, tui->find_text);
        break;
        
    case CmdFindRow:
        find_cell_text_row(tui, tui->cur_row, tui->cur_col, tui->find_text);
        break;
    
    }

}
