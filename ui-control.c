jump_cursor(unsigned row, unsigned col) {
    redraw_rows(CurRow, CurRow);              /* Clear Cursor */
    CurRow = row;
    CurCol = col;
    redraw_rows(CurRow, CurRow);               /* Draw Cursor */
}

move_cursor(int row, int col) {
    jump_cursor(max(0, (int)CurRow + row), max(0, (int)CurCol + col));
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
    
    read_csv(&TheTable, data, data + len);
    free(data);
    return 1;
}

save_csv(TCHAR *fn) {
    FILE *f = _wfopen(fn, L"wb");
    if (f) {
        write_csv(f, TheTable);
        fclose(f);
    }
    return !!f;
}
