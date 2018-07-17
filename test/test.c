#include <stdint.h>
#include <stdio.h>

int main() {
    int64_t a = 0;
    int64_t b = 1;
    int64_t c = 2;
    int64_t s[1] = {3};
    int64_t d = 4;

    printf("a: %d, %p\n", a, &a);
    printf("b: %d, %p\n", b, &b);
    printf("c: %d, %p\n", c, &c);
    printf("d: %d, %p\n", d, &d);

    printf("s[-3]: %d, %p\n", s[-3], &s[-3]);
    printf("s[-2]: %d, 5p\n", s[-2], &s[-2]);
    printf("s[-1]: %d, %p\n", s[-1], &s[-1]);
    printf("s[+0]: %d, %p\n", s[0], &s[0]);
    printf("s[+1]: %d, %p\n", s[1], &s[1]);
}