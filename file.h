static char *open_file(short *fn, unsigned *lenp) {
    FILE *f = _wfopen(fn, L"rb");
    char *data;
    if (!f) return 0;
    fseek(f, 0, SEEK_END);
    *lenp = ftell(f);
    rewind(f);
    fread(data = malloc(*lenp), 1, *lenp, f);
    fclose(f);
    return data;
}
static close_file(char *data) {
    free(data);
}