[BITS 32]
[SECTION .text]

[GLOBAL gdt_flush]
type gdt_flush function
gdt_flush:
	mov eax, [esp + 4]
	lgdt [eax]
	jmp 0x08:.flush
.flush:
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	ret

[GLOBAL idt_flush]
type idt_flush function
idt_flush:
	mov eax, [esp + 4]
	lidt [eax]
	ret

[GLOBAL tss_flush]
type tss_flush function
tss_flush:
	xor eax, eax
	mov eax, [esp + 4]
	ltr ax
	ret

section .note.GNU-stack