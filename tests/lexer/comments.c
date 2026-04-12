int main() {
    // single-line comment — everything here is ignored
    int x = 1;

    /* block comment
       spanning multiple lines
       int fake = 99;
    */
    int y = 2;

    int z = x + y; /* inline block comment */

    // nested-looking comment: /* this is just part of a line comment
    int w = z + 1;

    return w;
}
