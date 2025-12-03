extern void main(void);

void Reset_Handler(void) {
    main();
    while(1);
}

void Default_Handler(void) {
    while(1);
}

extern unsigned int _estack;

__attribute__ ((section(".isr_vector")))
void (*const vectors[])(void) = {
    (void (*)(void))(&_estack), // Initial stack pointer
    Reset_Handler,
};