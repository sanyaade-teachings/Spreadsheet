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
push_row(Table *table) {
    REALLOC(table->rows, ++table->n);
    table->rows[table->n - 1].n = 0;
    table->rows[table->n - 1].cells = 0;
}
push_col(Table *table, unsigned row) {
    if (row < row_count(table)) {
        Row *r = table->rows + row;
        REALLOC(r->cells, ++r->n);
        r->cells[r->n - 1].len = 0;
        r->cells[r->n - 1].str = 0;
    }
}
insert_row(Table *table, unsigned row) {
    if (row <= row_count(table)) {
        Row *r = REALLOC(table->rows, ++table->n) + row;
        memmove(r + 1, r, (row_count(table) - row - 1) * sizeof *r);
        r->n = 0;
        r->cells = 0;
    }
}
insert_cell(Table *table, unsigned row, unsigned col) {
    if (row <= row_count(table) && col <= col_count(table, row)) {
        Row *r = table->rows + row;
        Cell *c = REALLOC(r->cells, ++r->n) + col;
        memmove(c + 1, c, (col_count(table, row) - col - 1) * sizeof *c);
        c->len = 0;
        c->str = 0;
    }
}
clear_cell(Table *table, unsigned row, unsigned col) {
    if (col < col_count(table, row)) {
        Cell *cell = table->rows[row].cells + col;
        if (cell->str) free(cell->str);
        cell->len = 0;
        cell->str = 0;
    }
}
clear_row(Table *table, unsigned row) {
    if (row < row_count(table)) {
        unsigned n = col_count(table, row);
        while (n--) clear_cell(table, row, n);
    }
}
clear_rows(Table *table, unsigned row_lo, unsigned row_hi) {
    unsigned r;
    for (r = row_lo; r < row_hi; r++)
        clear_row(table, r);
}
clear_cells(Table *table, unsigned row_lo, unsigned row_hi, unsigned col_lo, unsigned col_hi) {
    unsigned r, c;
    for (r = row_lo; r < row_hi; r++)
        for (c = col_lo; c < col_hi; c++)
            clear_cell(table, r, c);
}
delete_cell(Table *table, unsigned row, unsigned col) {
    if (col < col_count(table, row)) {
        Cell *c = table->rows[row].cells + col;
        clear_cell(table, row, col);
        memmove(c, c + 1, (--table->rows[row].n - col) * sizeof *c);
    }
}
delete_row(Table *table, unsigned row) {
    if (row < row_count(table)) {
        Row *r = table->rows + row;
        clear_row(table, row);
        memmove(r, r + 1, (--table->n - row) * sizeof *r);
    }
}
delete_rows(Table *table, unsigned row_lo, unsigned row_hi) {
    unsigned r;
    for (r = row_lo; r < row_hi; r++)
        delete_row(table, row_lo);
}
delete_table(Table *table) {
    unsigned n = row_count(table);
    while (n) delete_row(table, --n);
}
delete_cells(Table *table, unsigned row_lo, unsigned row_hi, unsigned col_lo, unsigned col_hi) {
    unsigned r, c;
    for (r = row_lo; r < row_hi; r++)
        for (c = col_lo; c < col_hi; c++)
            delete_cell(table, r, col_lo);
}

Cell try_cell(Table *table, unsigned row, unsigned col) {
    return col < col_count(table, row)
        ? table->rows[row].cells[col]
        : empty_cell;
}
force_cell(Table *table, unsigned row, unsigned col) {
    while (row_count(table) <= row) push_row(table);
    while (col_count(table, row) <= col) push_col(table, row);
}
Cell get_cell(Table *table, unsigned row, unsigned col) {
    force_cell(table, row, col);
    return try_cell(table, row, col);
}
set_cell(Table *table, unsigned row, unsigned col, char *str, unsigned len) {
    Cell *cell;
    force_cell(table, row, col);
    cell = table->rows[row].cells + col;
    cell->str = memcpy(malloc(len+1), str, len + 1);
    cell->str[cell->len = len] = 0;
}
