/* Fixed-Point Arithmetic */
#define FRACTION    4
#define FIXNUM      10000
typedef long long Fixnum;

Fixnum add_fixnum(Fixnum a, Fixnum b) { return a + b; }
Fixnum sub_fixnum(Fixnum a, Fixnum b) { return a - b; }
Fixnum mul_fixnum(Fixnum a, Fixnum b) {
    return (a * b + 5*FIXNUM/10) / FIXNUM;
}
Fixnum div_fixnum(Fixnum a, Fixnum b) {
    if (b==0) return 0;
    return (a * FIXNUM*10 / b + 5) / 10;
}
char *spell_fixnum(char *buf, Fixnum x, int *len) {
    int n = sprintf(buf, "%s%lld.%0*u",
        (x < 0 && -FIXNUM < x)? "-": "",
        x/FIXNUM,
        FRACTION, abs(x)%FIXNUM);
    if (len) *len = n;
    return buf;
}
Fixnum parse_fixnum(char *text, char **textp) {
    Fixnum whole, frac = 0, neg = 1;
    unsigned n = FRACTION;
    text += strspn(text, " \t\r\n");
    if (*text == '-') neg = -1, text++;
    else if (*text == '+') neg = 1, text++;
    whole = strtol(text, &text, 10);
    if (*text == '.') {
        for (text++; n--; )
            frac = frac * 10 + (isdigit(*text)? *text++ - '0': 0);
        if (isdigit(*text) && *text >= '5') frac++;  /* Round */
        while (isdigit(*text)) text++;         /* Ignore rest */
    }
    if (textp) *textp = text;
    return neg * (whole * FIXNUM + frac);
}
