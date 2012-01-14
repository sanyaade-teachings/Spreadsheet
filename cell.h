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

#define REALLOC(X,N) X=realloc(X, (N)*sizeof *X)
row_count(Table *table) {
    return table->n;
}
col_count(Table *table, unsigned row) {
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