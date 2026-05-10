// expressions.c
// Exercises the full expression cascade: precedence, associativity, unary
// (prefix and postfix), assignment forms, ternary, calls, and indexing.
// Should parse with zero errors.

int square(int n) {
    return n * n;
}

int main() {
    int a, b, c;
    int x;
    int arr;          // used as if it were an array; parser does not check semantics

    // Arithmetic and precedence: a + b * c -> a + (b * c).
    a = 1 + 2 * 3;
    b = (a + 1) / 2;
    c = a - b - 1;    // left-associative: (a - b) - 1
    a = b % 3;

    // Unary prefix / postfix.
    a = -b;
    a = +b;
    a = ~b;
    x = !a;
    a = ++b;
    a = --b;
    a = b++;
    a = b--;

    // Pointer-style unary operators (parsed; semantics deferred).
    a = *p;
    a = &p;

    // Relational and equality (relational tighter than equality).
    x = a < b;
    x = a > b;
    x = a <= b;
    x = a >= b;
    x = a == b;
    x = a != b;

    // Shifts (additive tighter than shifts).
    x = a << 2;
    x = a >> 1;
    x = a << b + c;   // a << (b + c)

    // Bitwise (& tighter than ^ tighter than |).
    x = a & b;
    x = a ^ b;
    x = a | b;
    x = a | b & c;    // a | (b & c)

    // Logical (&& tighter than ||).
    x = a && b;
    x = a || b;
    x = a || b && c;  // a || (b && c)

    // Conditional (right-associative).
    x = a > 0 ? 1 : -1;
    x = a ? b : c ? 1 : 0;   // a ? b : (c ? 1 : 0)

    // Plain assignment (right-associative).
    a = b = c = 0;

    // Compound assignments — every form.
    a += 1;
    a -= 1;
    a *= 2;
    a /= 2;
    a %= 3;
    a &= 1;
    a |= 1;
    a ^= 1;
    a <<= 1;
    a >>= 1;

    // Calls.
    x = square(5);
    x = add(1, 2);             // forward use; parser does not verify
    x = square(square(3));     // nested

    // Indexing and chained postfix.
    a = arr[0];
    x = arr[a + 1];
    x = arr[0]++;              // ((arr[0]))++

    // Parenthesised grouping (no ParenExpr — grouping is structural).
    x = (a + b) * c;
    x = ((a));

    return 0;
}
