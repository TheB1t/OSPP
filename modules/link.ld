OUTPUT_FORMAT(elf32-i386)

SECTIONS {
	/DISCARD/ : { 
		*(.comment) 
		*(.note*) 
		*(.eh_frame*) 
		*(.eh_frame_hdr)
	}
}