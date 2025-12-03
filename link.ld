MEMORY {
    FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 32K
    SRAM (rwx) : ORIGIN = 0x10000000, LENGTH = 8K
}
_estack = ORIGIN(SRAM) + LENGTH(SRAM);

SECTIONS {
    .text : { KEEP(*(.isr_vector)) *(.text*) } > FLASH
    .bss : { *(.bss*) } > SRAM
}