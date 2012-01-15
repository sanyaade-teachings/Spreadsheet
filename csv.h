#include <assert.h>
#define CELLLEN 65536

#define IS_EOL 
Table *read_csv(Table *table, unsigned row, unsigned left, char *f, char *eof, unsigned *max_rowp, unsigned *max_colp) {
    unsigned col = left, max_row, max_col = left;
    
    row--;
record:
    max_col = max(col, max_col);
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
    if (max_rowp) *max_rowp = row;
    if (max_colp) *max_colp = max_col;
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