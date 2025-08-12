OUTPUT_FORMAT(elf32-i386)

SECTIONS {
    __module_start = .;

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

    __module_end = .;
}