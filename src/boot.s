MBOOT_PAGE_ALIGN    equ 1 << 0
MBOOT_MEM_INFO      equ 1 << 1
MBOOT_VBE_MODE      equ 1 << 2
MBOOT_HEADER_MAGIC  equ 0x1BADB002
MBOOT_HEADER_FLAGS  equ MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO
MBOOT_CHECKSUM      equ -(MBOOT_HEADER_MAGIC + MBOOT_HEADER_FLAGS)

[BITS 32]

[GLOBAL stack]
[GLOBAL mboot]
[EXTERN _data]
[EXTERN _bss]
[EXTERN _end]
[EXTERN _start]

section .multiboot

mboot:
	dd  MBOOT_HEADER_MAGIC
	dd  MBOOT_HEADER_FLAGS
	dd  MBOOT_CHECKSUM

	dd  mboot
	dd  _data
	dd  _bss
	dd  _end
	dd  _start

section .bss

align 16
stack:
	resb 4096
  .top:

section .text

[GLOBAL start]
[EXTERN kernel_early_main]
type start function
start:
	xor ebp, ebp		; For stack unwinding
	mov esp, stack.top
	and esp, 0xFFFFFFF0 ; System V ABI requires 16-byte stack alignment

	push eax
	push ebx

	cli
	call kernel_early_main

.hang:
	hlt
	jmp .hang

section .note.GNU-stack