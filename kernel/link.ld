ENTRY(cold_start)

OUTPUT_FORMAT(elf32-i386)

SECTIONS {
	/* First put the multiboot header, as it is required to be put very early
	   in the image or the bootloader won't recognize the file format.
	   Next we'll put the .text section. */
    .multiboot : {
        *(.multiboot)
    }

    /*
        Trampoline at 8000
    */
    .trampoline 0x8000 : AT(0x8000) {
        KEEP(*(.trampoline))
    }

    . = 2M;

    __kernel_start = .;

	.text : ALIGN(4K) {
		*(.text)
	}

	/* Read-write data (initialized) && read-only data */
	.data : ALIGN(4K) {
		*(.data)
        *(.rodata)
	}

	/* Read-write data (uninitialized) and stack */
	.bss : ALIGN(4K) {
		*(COMMON)
		*(.bss)
	}

    .init_array : {
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array))
    }

    .fini_array : {
        KEEP (*(SORT(.fini_array.*)))
        KEEP (*(.fini_array))
    }

    __kernel_end = .;
}