%include "./include/nasm_macros.inc"

global loader_switch_stack_pointers
global idle_main

%define old_stack_top_ptr [ebp + 8]
%define new_stack_top_ptr [ebp + 12]
loader_switch_stack_pointers:
    push ebp
    mov ebp, esp
    
    mov eax, old_stack_top_ptr
    mov ecx, new_stack_top_ptr
    
    ; Guardamos el esp de la tarea vieja
    mov [eax], esp

    ; Cargamos el esp de la tarea a la que vamos
    mov esp, [ecx]

    pop ebp

    ret

idle_main:
    ;hlt
    xchg bx, bx
    jmp idle_main
