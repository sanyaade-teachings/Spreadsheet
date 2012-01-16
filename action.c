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
    
};

scroll(int row, int col);

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

command(int cmd) {

    switch (cmd) {
    
    case CmdClearFile: clear_file(); break;
    case CmdOpenFile:  clear_and_open(TheFilename); break;    
    case CmdSaveFile:
        if (!save_csv(TheFilename))
            MessageBox(TheWindow, L"Could not save the file", L"Error", MB_OK);
        break;
    
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
    }

}
