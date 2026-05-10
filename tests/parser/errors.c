
// errors.c
// Intentional syntax errors exercising the parser's recovery via synchronize().
// The parser should record one or more ParseError records and keep going past
// each error rather than aborting. The accompanying test asserts that the error
// count is non-zero.

// (1) Missing ';' after declaration. The parser reports an error at the next
//     non-';' token; synchronize() advances to the next safe boundary.
int x = 5

// (2) Same-scope redeclaration: declare() returns false on (scope=0, "x").
int x;

// (3) Missing identifier after the type specifier.
int = 99;

// (4) Missing ')' in if-condition. Recovery resumes at the next statement-
//     leading keyword or brace.
int withBadIf() {
    int y = 0;
    if (y > 0
        y = 1;
    return y;
}

// (5) Missing ';' in the middle of a function body.
int withMissingSemi() {
    int z = 1
    return z;
}

// (6) Bare operator with no right-hand side. parsePrimary reports
//     "expected expression" and skips one token.
int withBareOp() {
    int a = 0;
    a = 2 + ;
    return a;
}

// (7) Missing closing brace at end of a function. Triggers an unterminated-
//     block error. Placed last so trailing recovery doesn't disturb earlier
//     productions.
int withMissingBrace() {
    int q = 1;
    return q;
