int main() {
    int a = 5;
    int b = 3;

    int add  = a + b;
    int sub  = a - b;
    int mul  = a * b;
    int div  = a / b;
    int mod  = a % b;

    int eq   = a == b;
    int neq  = a != b;
    int lt   = a < b;
    int gt   = a > b;
    int le   = a <= b;
    int ge   = a >= b;

    int and  = a && b;
    int or   = a || b;
    int not  = !a;

    int band = a & b;
    int bor  = a | b;
    int bxor = a ^ b;
    int bnot = ~a;
    int lsh  = a << 1;
    int rsh  = a >> 1;

    a += b;
    a -= b;
    a *= b;
    a /= b;
    a %= b;
    a &= b;
    a |= b;
    a ^= b;
    a <<= 1;
    a >>= 1;

    int t = a ? a : b;

    return 0;
}
