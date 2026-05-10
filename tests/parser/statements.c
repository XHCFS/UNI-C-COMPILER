// statements.c
// Exercises every statement form the parser recognises.
// Should parse with zero errors.

int main() {
    int x;
    int i;
    int sum;

    // Expression statement.
    x = 0;

    // Empty statement (a stray ';' produces no AST node — silently skipped).
    ;
    ;

    // if without else.
    if (x == 0) x = 1;

    // if-else.
    if (x > 0)
        x = x + 1;
    else
        x = x - 1;

    // Dangling else: 'else' binds to the nearest unmatched 'if'.
    if (x > 0)
        if (x > 10)
            x = 10;
        else
            x = x;          // matches inner 'if'

    // Nested compound block introduces a new scope.
    {
        int local = 5;
        x = x + local;
    }

    // Pre-test loop.
    i = 0;
    while (i < 10) {
        i = i + 1;
    }

    // Post-test loop.
    i = 0;
    do {
        i = i + 1;
    } while (i < 5);

    // for with declaration init (i is scoped to the loop).
    sum = 0;
    for (int j = 0; j < 10; j = j + 1) {
        sum = sum + j;
    }

    // for with expression init and break/continue inside.
    for (i = 0; i < 5; i = i + 1) {
        if (i == 3) break;
        if (i == 1) continue;
        x = x * 2;
    }

    // for with all clauses empty.
    for (;;) break;

    // Bare return.
    if (x < 0) return;

    // return with value.
    return x;
}
