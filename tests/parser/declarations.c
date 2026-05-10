// declarations.c
// Variable and function declarations exercised by the parser.
// Should parse with zero errors.

// Simple variable declarations.
int   x;
int   y = 10;
float pi = 3.14;
char  letter;

// Comma-chained declarators (each gets its own VarDecl).
int a, b, c;

// Pointers (depth resets per declarator).
int   *p;
char  *s;
int   *pa, qa;

// Type qualifiers.
const int    MAX = 100;
volatile int counter;
const volatile int both = 0;

// Storage-class keywords (consumed and discarded by the parser, not stored on Type).
static int privateCount;
extern int externalVar;

// Sign and length modifiers (also consumed but not surfaced on Type in v1).
unsigned int  positive = 1;
signed   int  negative = -1;
long          big = 1000;
short         small_count;
long long     huge;

// Function definitions with various parameter shapes.
int square(int n) {
    return n * n;
}

int add(int a, int b) {
    return a + b;
}

int noArgs() {
    return 0;
}

int explicitVoid(void) {
    return 1;
}

// Pointer parameter and return type.
int firstByte(char *buf) {
    return 0;
}

int main() {
    int local = 0;
    int result;
    result = add(1, 2);
    return result;
}
