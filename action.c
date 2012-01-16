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

scroll(int row, int col);
auto_resize_column(unsigned col);

insert_datetime(int include_time) {
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
    set_cell(&TheTable, CurRow, CurCol, timestr, strlen(timestr));
    redraw_rows(CurRow, CurRow);
}

cell_has_text(Table *table, unsigned row, unsigned col, char *text) {
    Cell cell = try_cell(table, row, col);
    return !!strstr(cell.str, text);
}

find_cell_text_col(Table *table, unsigned row, unsigned col, char *text) {
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
    jump_cursor(r, c);
    return 1;
}

find_cell_text_row(Table *table, unsigned row, unsigned col, char *text) {
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
    jump_cursor(r, c);
    return 1;
}

command(int cmd) {

    switch (cmd) {
    
    case CmdClearFile: clear_file(); break;
    case CmdOpenFile:
        clear_and_open(TheFilename);
        if (row_count(&TheTable) < MAX_ROWS_FOR_FIT) {
            unsigned i;
            for (i = 0; i < col_count(&TheTable, 0); i++)
                auto_resize_column(i);
        }
        break;
    case CmdSaveFile: save_csv(TheFilename); break;
    
    case CmdSetAnchor: set_anchor(); break;
    case CmdClearAnchor: clear_anchor(); break;
    
    case CmdClearRow: clear_selected_rows(); break;
    case CmdDeleteRow: delete_selected_rows(); break;
    case CmdDeleteCell: delete_selected_cells(); break;
    case CmdClearCell: clear_selected_cells(); break;
    
    case CmdCopy: copy_to_clipboard(); break;
    case CmdCutClear: copy_to_clipboard(); clear_selected_cells(); break;
    case CmdCutDelete: copy_to_clipboard(); delete_selected_cells(); break;
    case CmdPaste: clear_anchor(); paste_clipboard(); break;
    
    case CmdReturn: clear_anchor(); move_cursor(1, 0); break;
    case CmdTab: clear_anchor(); move_cursor(0, 1); break;
    case CmdUnReturn: clear_anchor(); move_cursor(-1, 0); break;
    case CmdUnTab: clear_anchor(); move_cursor(0, -1); break;
    
    case CmdEditCell: start_edit(1); break;
    case CmdEditCellClear: start_edit(0); break;
    case CmdCommitEdit: end_edit(); break;
    case CmdCancelEdit: cancel_edit(); break;
    
    case CmdMoveUp: move_cursor(-1, 0); break;
    case CmdMoveDown: move_cursor(1, 0); break;
    case CmdMoveLeft: move_cursor(0, -1); break;
    case CmdMoveRight: move_cursor(0, 1); break;
    
    case CmdScrollUp: scroll(-1, 0); break;
    case CmdScrollDown: scroll(1, 0); break;
    case CmdScrollLeft: scroll(0, -1); break;
    case CmdScrollRight: scroll(0, 1); break;
    
    case CmdHomeCol: jump_cursor(0, CurCol); break;
    case CmdHomeRow: jump_cursor(CurRow, 0); break;
    case CmdEndCol: jump_cursor(row_count(&TheTable) - 1, CurCol); break;
    case CmdEndRow: jump_cursor(CurRow, col_count(&TheTable, CurRow) - 1); break;
    
    case CmdInsertDate: insert_datetime(0); break;
    case CmdInsertDateTime: insert_datetime(1); break;
    
    case CmdInsertCell:
        insert_cells(&TheTable, CurRow, CurCol, 1);
        redraw_rows(CurRow, CurRow);
        break;
    case CmdInsertRow:
        insert_rows(&TheTable, CurRow, 1);
        redraw_rows(CurRow, -1);
        break;
    
    case CmdFindColumn:
        find_cell_text_col(&TheTable, CurRow, CurCol, TheFindText);
        break;
        
    case CmdFindRow:
        find_cell_text_row(&TheTable, CurRow, CurCol, TheFindText);
        break;
    
    }

}
