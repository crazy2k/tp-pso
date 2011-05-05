%macro c_enter 0-1 0
    push ebp
    mov ebp, esp
    %if %1 != 0
        sub esp, %1
    %endif
    push edi
    push esi
    push ebx
%endmacro

%macro c_leave 0-1 0
    pop ebx
    pop esi
    pop edi
    %if %1 != 0
        add esp, %1
    %endif
    pop ebp

    ret
%endmacro

; Llama a la funcion de C en %1 pasandole los parametros %2, %3, ...
%macro CCALL 1-*
    %rep (%0 - 1)
        %rotate -1
        push %1
    %endrep
    %rotate -1
    call %1
    add esp, 4*(%0 - 1) ; asumimos que los parametros son de 32 bits
%endmacro

