#include <stdint.h>

int main(void) {
    volatile uint32_t i;
    while (1) {
        for (i = 0; i < 100000; i++);
    }
    return 0;
}