%include "./include/nasm_macros.inc"

extern idt_handle

global idt_stateful_handlers

%define IDT_LENGTH      256
%define IDT_LAST_INDEX  (IDT_LENGTH - 1)


; Macro que crea un stateful handler para una entrada en la IDT.
; - %1 es el indice de la IDT en el que se ubicara una referencia a el
; - %2 es 1 si la excepcion admite un codigo de error y 0 si no; este
;   parametro es opcional y por omision vale 0.
%define ST_ESP [esp + 3*4]
%define ST_ORG_ESP [esp + 16*4]
%define ST_ORG_SS [esp + 17*4]
%define ST_CS [esp + 14*4]
%define ST_SS [esp + 12*4]
%macro CREATE_STATEFUL_HANDLER 1-2 0
idt_handler_%1:
    ; Guardamos el codigo de error, si lo hay
    %if %2 != 0
        pop dword [error_code_%1]
    %endif

    ; Salvamos el estado en la pila
    push ss
    push ds
    push es
    push fs
    push gs
    pusha

    ; Verificamos si ocurrio un cambio de nivel
    test dword ST_CS, 0x00000003
    je no_level_change_%1
    ;   si hubo cambio de nivel, hacemos ss = org_ss y esp = org_esp
    mov eax, ST_ORG_ESP
    mov ST_ESP, eax
    mov eax, ST_ORG_SS
    mov ST_SS, eax

no_level_change_%1:
    ;   si no hubo cambio de nivel, el ss esta bien puesto y calculamos el esp
    lea eax, [esp + 16*4]
    mov ST_ESP, eax

    ; Llamamos a la funcion que manejara la excepcion/interrupcion
    push esp
    push dword [error_code_%1]
    push %1
    call idt_handle
    add esp, 3*4

    jmp load_state

error_code_%1: dd 0x0
%endmacro

; Creamos IDT_LENGTH stateful handlers
%assign i 0
%rep IDT_LENGTH
    %if i == 8 || i == 10 || i == 11 || i == 12 || i == 13 || i == 14 || i == 17
        CREATE_STATEFUL_HANDLER i, 1
    %else
        CREATE_STATEFUL_HANDLER i
    %endif
    %assign i (i + 1)
%endrep

; Arreglo de stateful handlers para cada indice en la IDT.
idt_stateful_handlers:
%assign i 0
%rep IDT_LENGTH
    dd idt_handler_ %+ i
    %assign i (i + 1)
%endrep

; Carga el estado guardado en la pila.
load_state:
    popa
    pop gs
    pop fs
    pop es
    pop ds
    pop ss

    iret


