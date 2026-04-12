int main() {
    const int    ci = 10;
    static int   si = 20;
    unsigned int ui = 30;
    signed   int ni = -5;
    long     int li = 1000;
    short    int hi = 1;
    double       d  = 2.71;
    float        f  = 1.41;
    char         ch = 'z';
    void*        p  = 0;

    typedef int MyInt;
    MyInt val = ci + si;

    extern int external_var;
    volatile int vi = 0;

    int size = sizeof(int);

    return 0;
}
