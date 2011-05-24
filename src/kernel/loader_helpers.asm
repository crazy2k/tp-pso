%include "./include/nasm_macros.inc"

global loader_switch_stack_pointers
global idle_main
global initialize_task

extern loader_setup_task_memory
extern load_state

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
    hlt
    jmp idle_main



initialize_task:
    ; El pso_file * ya se encuentra en el stack, por lo que no hace falta
    ; hacer push para pasarlo como parametro
    CCALL loader_setup_task_memory
    add esp, 4

    ; Ahora que tenemos listo nuestro espacio de memoria virtual, cargamos el
    ; estado inicial de la tarea
    jmp load_state
