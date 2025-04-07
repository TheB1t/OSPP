ENTRY(start)

OUTPUT_FORMAT(elf32-i386)

SECTIONS {
    . = 2M;

    _start = .; __kernel_start = .;

	/* First put the multiboot header, as it is required to be put very early
	   in the image or the bootloader won't recognize the file format.
	   Next we'll put the .text section. */
    .multiboot : {
        *(.multiboot)
    }

	.text : ALIGN(4K) {
        _code = .;
		*(.text)
	}

	/* Read-write data (initialized) && read-only data */
	.data : ALIGN(4K) {
        _data = .;
		*(.data)
        *(.rodata)
	}

	/* Read-write data (uninitialized) and stack */
	.bss : ALIGN(4K) {
        _bss = .;
		*(COMMON)
		*(.bss)
	}

    .init_array : {
        crti.o(.init_array)
        KEEP (*(SORT(EXCLUDE_FILE(crti.o crtn.o) .init_array.*)))
        KEEP (*(EXCLUDE_FILE(crti.o crtn.o) .init_array))
        crtn.o(.init_array)
    }

    .fini_array : {
        crti.o(.fini_array)
        KEEP (*(SORT(EXCLUDE_FILE(crti.o crtn.o) .fini_array.*)))
        KEEP (*(EXCLUDE_FILE(crti.o crtn.o) .fini_array))
        crtn.o(.fini_array)
    }

    _end = .; __kernel_end = .;
}