typedef struct {
    unsigned len;
    char    *str;
} Cell;
typedef struct {
    unsigned n;
    Cell    *cells;
} Row;
typedef struct {
    unsigned n;
    unsigned max_col;
    Row     *rows;
} Table;

Cell    empty_cell = {0, ""};

#define REALLOC(X,N) (X=realloc(X, (N)*sizeof *X))
unsigned row_count(Table *table) {
    return table->n;
}
unsigned col_count(Table *table, unsigned row) {
    return row < row_count(table)? table->rows[row].n: 0;
}
unsigned max_col_count(Table *table) {
    return table->max_col;
}
insert_rows(Table *table, unsigned row, unsigned n) {
    if (row <= row_count(table)) {
        Row *r = REALLOC(table->rows, (table->n += n)) + row;
        memmove(r + n, r, (row_count(table) - row - n) * sizeof *r);
        memset(r, 0, n * sizeof *r);
    }
}
insert_cells(Table *table, unsigned row, unsigned col, unsigned n) {
    if (row <= row_count(table) && col <= col_count(table, row)) {
        Row *r = table->rows + row;
        Cell *c = REALLOC(r->cells, (r->n += n)) + col;
        memmove(c + n, c, (col_count(table, row) - col - n) * sizeof *c);
        memset(c, 0, n * sizeof *c);
        table->max_col = max(table->max_col, r->n);
    }
}
clear_cells(Table *table, unsigned row, unsigned col, unsigned n) {
    if (col_count(table, row)) {
        Cell *c = table->rows[row].cells + col;
        n = min(n, col_count(table, row) - col);
        for ( ; n; c++, n--) {
            if (c->str) free(c->str);
            c->len = 0;
            c->str = 0;
        }
    }
}
clear_rows(Table *table, unsigned row, unsigned n) {
    for ( ; n; n--, row++)
        clear_cells(table, row, 0, col_count(table, row));
}

calc_max_col(Table *table) {
    unsigned r;
    table->max_col = 0;
    for (r = 0; r < row_count(table); r++)
        table->max_col = max(table->max_col, col_count(table, r));
    return table->max_col;
}

delete_cells(Table *table, unsigned row, unsigned col, unsigned n){
    if (col < col_count(table, row)) {
        Cell *c = table->rows[row].cells + col;
        unsigned old_count = col_count(table, row);
        clear_cells(table, row, col, n);
        n = min(n, col_count(table, row) - col);
        table->rows[row].n -= n;
        memmove(c, c + n, (col_count(table, row) - col) * sizeof *c);
        
        if (old_count == max_col_count(table))
            calc_max_col(table);
    }
}
delete_rows(Table *table, unsigned row, unsigned n) {
    clear_rows(table, row, n);
    if (row < row_count(table)) {
        unsigned old_count = col_count(table, row);
        Row *r = table->rows + row;
        n = min(n, row_count(table) - row);
        table->n -= n;
        memmove(r, r + n, (row_count(table) - row) * sizeof *r);
        
        if (old_count == max_col_count(table))
            calc_max_col(table);
    }
}
delete_table(Table *table) {
    delete_rows(table, 0, row_count(table));
}


Cell try_cell(Table *table, unsigned row, unsigned col) {
    return col < col_count(table, row)
        ? table->rows[row].cells[col]
        : empty_cell;
}
force_cell(Table *table, unsigned row, unsigned col) {
    if (row_count(table) <= row)
        insert_rows(table, row_count(table),
            row - row_count(table) + 1);
    if (col_count(table, row) <= col)
        insert_cells(table, row, col_count(table, row),
            col - col_count(table, row) + 1);
}
Cell get_cell(Table *table, unsigned row, unsigned col) {
    force_cell(table, row, col);
    return try_cell(table, row, col);
}
set_cell(Table *table, unsigned row, unsigned col, char *str, unsigned len) {
    Cell *cell;
    force_cell(table, row, col);
    cell = table->rows[row].cells + col;
    if (cell->str) free(cell->str);
    cell->str = memcpy(malloc(len+1), str, len + 1);
    cell->str[cell->len = len] = 0;
}
