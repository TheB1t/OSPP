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

    .kexp : {
        __kexp_start = .;
        KEEP (*(.kexp*))
        __kexp_end   = .;
    }

	.text : ALIGN(4K) {
		*(.text*)
	}

	/* Read-write data (initialized) && read-only data */
	.data : ALIGN(4K) {
		*(.data*)
        *(.rodata*)
	}

	/* Read-write data (uninitialized) and stack */
	.bss : ALIGN(4K) {
		*(COMMON)
		*(.bss*)
	}

    __kernel_end = .;

	/DISCARD/ : { 
		*(.comment) 
		*(.note*) 
		*(.eh_frame*) 
		*(.eh_frame_hdr) 
	}
}
