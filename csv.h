#include <assert.h>
#define CELLLEN 65536

#define IS_EOL 
Table *read_csv(Table *table, unsigned row, unsigned left, char *f, char *eof) {
    unsigned col = left;
    
    row--;
record:
    col = left;
    row++;
    
    while (f < eof)
        if (*f == '"') {
            char buf[CELLLEN], *p = buf, *cap = buf + CELLLEN;
            for (f++; f < eof && p < cap; f++)
                if (*f == '"') {
                    if (f+1 < eof && f[1] == '"')
                        *p++ = *f++;
                    else
                        break;
                } else
                    *p++ = *f;
            
            assert(p < cap);
            set_cell(table, row, col++, buf, p - buf);
            
            if (*f == '"') f++;
            if (*f == ',') f++;
            if (*f == '\r' || *f == '\n') {
                if (*f == '\r') f++;
                if (*f == '\n') f++;
                goto record;
            }
        } else {
            char *start = f;
            for (;;)
                if (eof <= f) {
                    set_cell(table, row, col++, start, f - start);
                    break;
                } else if (*f == ',') {
                    set_cell(table, row, col++, start, f - start);
                    f++;
                    break;
                } else if (*f == '\r' || *f == '\n') {
                    set_cell(table, row, col++, start, f - start);
                    if (*f == '\r') f++;
                    if (*f == '\n') f++;
                    goto record;
                } else
                    f++;
        }
    return table;
}
static need_escape(unsigned char *text, unsigned len) {
    static char map[256];
    if (!map['"']) map['"'] = map['\r'] = map['\n'] = map[','] = 1;
    for ( ; len && !map[*text++]; len--);
    return len != 0;
}

write_csv_cell(FILE *f, Cell cell) {
    if (cell.str)
        if (need_escape(cell.str, cell.len)) {
            char *p;
            putc('"', f);
            for (p = cell.str; p < cell.str + cell.len; p++) {
                if (*p == '"') putc('"', f);
                putc(*p, f);
            }
            putc('"', f);
        } else
            fwrite(cell.str, 1, cell.len, f);
}
write_csv_row(FILE *f, Row row) {
    for ( ; row.n; row.n--) {
        write_csv_cell(f, *row.cells++);
        if (row.n != 1) putc(',', f);
    }
}
write_csv(FILE *f, Table table) {
    for ( ; table.n; table.n--) {
        write_csv_row(f, *table.rows++);
        if (table.n != 1) fputs("\r\n", f);
    }
}
write_csv_cells(FILE *f, Table *table, unsigned row_lo, unsigned row_hi, unsigned col_lo, unsigned col_hi) {
    unsigned r, c;
    for (r = row_lo; r < row_hi; r++) {
        for (c = col_lo; c < col_hi; c++) {
            write_csv_cell(f, try_cell(table, r, c));
            if (c + 1 < col_hi) putc(',', f);
        }
        if (r + 1 < row_hi) fputs("\r\n", f);
    }
}