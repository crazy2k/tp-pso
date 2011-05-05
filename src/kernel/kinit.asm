bits 16
%include "bios.mac"
section .text

%define seg_code_0 8
%define seg_data_0 16
%define GDT_COUNT 32 ; WARNING, must match value defined in gdt.h
extern kernel_init

; Compatibilidad con codigo de Zafio
extern gdtr
GDT_DESC: equ gdtr

extern enable_A20

global start
global debug_backtrace_limit
; start MUST be at the very begining of this file
start: 
	call enable_A20

	lgdt [GDT_DESC]

	mov eax, cr0
	or eax, 0x1
	mov cr0, eax

	jmp 0x08:modo_protegido

BITS 32
modo_protegido:
	mov 	ax, 0x10
	mov 	ds, ax
	mov 	es, ax
	mov 	fs, ax
	mov 	ss, ax
	mov 	esp, 0xA0000
	mov 	ebp, 0xA0000

debug_backtrace_limit:
	call 	kernel_init
	jmp $

