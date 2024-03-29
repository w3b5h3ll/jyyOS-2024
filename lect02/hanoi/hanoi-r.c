#include <stdio.h>

int hanoi(int n, char from, char to, char via) {
    if (n == 1) {
        printf("%c -> %c\n", from, to);
        return 1;
    } else {
        // 将n-1个从from移动到via
        int c1 = hanoi(n - 1, from, via, to);
        // 将最后一个从from移动到to
        hanoi(1, from, to, via);
        // 将n-1个从via移动到to
        int c2 = hanoi(n - 1, via, to, from);
        return c1 + c2 + 1;
    }
}
