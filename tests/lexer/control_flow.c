int main() {
    int x = 10;

    if (x > 0) {
        x = x - 1;
    } else {
        x = x + 1;
    }

    while (x > 0) {
        x -= 1;
    }

    do {
        x += 1;
    } while (x < 5);

    int i;
    for (i = 0; i < 10; i++) {
        if (i == 5) {
            break;
        }
        if (i == 3) {
            continue;
        }
    }

    switch (x) {
        case 1:
            x = 10;
            break;
        case 2:
            x = 20;
            break;
        default:
            x = 0;
            break;
    }

    return x;
}
