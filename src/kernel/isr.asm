;; isr.asm
;;

global gather_and_panic_noerrcode
global gather_and_panic_errcode

extern debug_kernelpanic

%macro SAVE_REGS 20
    %assign displacement 0
    %assign i 0
    %rep (%0 - 1)
        mov [%1 + displacement], %i

        %assign displacement (displacement + 4)
        %assign i (i + 1)
    %endrep
%endmacro

%macro SAVE_REGS 0-1 0
    ; Si la excepcion no admite codigo de error, escribimos cero como codigo
    ; de error
    %if %1 == 0
        push dword 0x0
    %endif

    ; Salvamos esp
    push esp

    ; Guardamos todos los registros
    push ss
    push esp
    pushfd
    push cs
    push dword [esp - 6*4]
    push dword [esp - 7*4]    ; errcode
    push eax
    push ecx
    push edx
    push ebx
    push dword 0x0    ; placeholder esp
    push ebp
    push esi
    push edi
    push ss
    push ds
    push es
    push fs
    push gs

    ; Escribimos el valor de esp correcto
    mov ebx, [esp - 19*4]
    sub ebx, 8
    mov [esp - 8*4], ebx

    ; Pasamos los parametros a la funcion de C
    push esp
    push ebx
    call debug_kernelpanic


%endmacro

gather_and_panic_noerrcode:
    SAVE_REGS

gather_and_panic_errcode:
    SAVE_REGS 1


