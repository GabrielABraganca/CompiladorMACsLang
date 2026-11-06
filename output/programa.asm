; =============================================================
; Codigo gerado automaticamente pelo Compilador MACSLang
; Arquitetura: x86 (32 bits) | Sistema: Windows
; Montador:    NASM
; =============================================================

; Para compilar e linkar (Windows):
;   nasm -f win32 programa.asm -o programa.obj
;   gcc -m32 programa.obj -o programa.exe

global _main
extern _printf
extern _scanf

section .data
    fmt_int    db "%d", 10, 0
    fmt_float  db "%f", 10, 0
    fmt_str    db "%s", 10, 0
    fmt_char   db "%c", 10, 0
    fmt_in_int   db "%d", 0
    fmt_in_float db "%f", 0
    fmt_in_str   db "%s", 0

section .text


; --- Funcao: soma ---
_soma:
    ; Prologo da funcao
    push ebp
    mov ebp, esp
    sub esp, 512  ; espaco para variaveis locais
    ; return
    ; Operacao binaria '+'
    ; Carregar variavel 'a'
    mov eax, [_a]
    push eax
    ; Carregar variavel 'b'
    mov eax, [_b]
    mov ebx, eax
    pop eax
    add eax, ebx
    mov esp, ebp
    pop ebp
    ret
    ; Epilogo da funcao
    xor eax, eax
    mov esp, ebp
    pop ebp
    ret

; --- Funcao: maior ---
_maior:
    ; Prologo da funcao
    push ebp
    mov ebp, esp
    sub esp, 512  ; espaco para variaveis locais
    ; if
    ; Operacao binaria '>'
    ; Carregar variavel 'a'
    mov eax, [_a]
    push eax
    ; Carregar variavel 'b'
    mov eax, [_b]
    mov ebx, eax
    pop eax
    cmp eax, ebx
    setg al
    movzx eax, al
    cmp eax, 0
    je .else_0
    ; return
    ; Carregar variavel 'a'
    mov eax, [_a]
    mov esp, ebp
    pop ebp
    ret
    jmp .endif_0
.else_0:
    ; return
    ; Carregar variavel 'b'
    mov eax, [_b]
    mov esp, ebp
    pop ebp
    ret
.endif_0:
    ; Epilogo da funcao
    xor eax, eax
    mov esp, ebp
    pop ebp
    ret

; --- Funcao: potencia ---
_potencia:
    ; Prologo da funcao
    push ebp
    mov ebp, esp
    sub esp, 512  ; espaco para variaveis locais
    ; Declaracao de variavel 'resultado'
    sub esp, 4
    ; Literal inteiro: 1
    mov eax, 1
    mov [ebp - 4], eax
    ; Declaracao de variavel 'i'
    sub esp, 4
    ; Literal inteiro: 0
    mov eax, 0
    mov [ebp - 8], eax
    ; while
.while_start_1:
    ; Operacao binaria '<'
    ; Carregar variavel 'i'
    mov eax, [_i]
    push eax
    ; Carregar variavel 'exp'
    mov eax, [_exp]
    mov ebx, eax
    pop eax
    cmp eax, ebx
    setl al
    movzx eax, al
    cmp eax, 0
    je .while_end_1
    ; Atribuicao a 'resultado'
    ; Operacao binaria '*'
    ; Carregar variavel 'resultado'
    mov eax, [ebp - 4]
    push eax
    ; Carregar variavel 'base'
    mov eax, [_base]
    mov ebx, eax
    pop eax
    imul eax, ebx
    mov [ebp - 4], eax
    ; Atribuicao a 'i'
    ; Operacao binaria '+'
    ; Carregar variavel 'i'
    mov eax, [_i]
    push eax
    ; Literal inteiro: 1
    mov eax, 1
    mov ebx, eax
    pop eax
    add eax, ebx
    mov [_i], eax
    jmp .while_start_1
.while_end_1:
    ; return
    ; Carregar variavel 'resultado'
    mov eax, [ebp - 4]
    mov esp, ebp
    pop ebp
    ret
    ; Epilogo da funcao
    xor eax, eax
    mov esp, ebp
    pop ebp
    ret

; --- Funcao: ehPositivo ---
_ehPositivo:
    ; Prologo da funcao
    push ebp
    mov ebp, esp
    sub esp, 512  ; espaco para variaveis locais
    ; if
    ; Operacao binaria '>'
    ; Carregar variavel 'n'
    mov eax, [_n]
    push eax
    ; Literal inteiro: 0
    mov eax, 0
    mov ebx, eax
    pop eax
    cmp eax, ebx
    setg al
    movzx eax, al
    cmp eax, 0
    je .else_2
    ; return
    ; Booleano: true
    mov eax, 1
    mov esp, ebp
    pop ebp
    ret
    jmp .endif_2
.else_2:
.endif_2:
    ; return
    ; Booleano: false
    mov eax, 0
    mov esp, ebp
    pop ebp
    ret
    ; Epilogo da funcao
    xor eax, eax
    mov esp, ebp
    pop ebp
    ret

; === Ponto de entrada do programa ===
_main:
    ; Prologo do main
    push ebp
    mov ebp, esp
    sub esp, 512  ; espaco para variaveis locais do main
    ; print(...)
    ; String literal
    jmp .str_end_3
  .str_data_3 db , "=== Calculadora MACSLang ===", 0
  .str_end_3:
    lea eax, [.str_data_3]
    push eax
    push fmt_str
    call _printf
    add esp, 8
    ; Declaracao de variavel 'a'
    sub esp, 4
    mov dword [ebp - 4], 0
    ; Declaracao de variavel 'b'
    sub esp, 4
    mov dword [ebp - 8], 0
    ; print(...)
    ; String literal
    jmp .str_end_4
  .str_data_4 db , "Digite o primeiro numero:", 0
  .str_end_4:
    lea eax, [.str_data_4]
    push eax
    push fmt_str
    call _printf
    add esp, 8
    ; input(...)
    lea eax, [ebp - 4]
    push eax
    push fmt_in_int
    call _scanf
    add esp, 8
    ; print(...)
    ; String literal
    jmp .str_end_5
  .str_data_5 db , "Digite o segundo numero:", 0
  .str_end_5:
    lea eax, [.str_data_5]
    push eax
    push fmt_str
    call _printf
    add esp, 8
    ; input(...)
    lea eax, [ebp - 8]
    push eax
    push fmt_in_int
    call _scanf
    add esp, 8
    ; Declaracao de variavel 's'
    sub esp, 4
    ; Chamada de funcao 'soma'
    ; Carregar variavel 'b'
    mov eax, [ebp - 8]
    push eax
    ; Carregar variavel 'a'
    mov eax, [ebp - 4]
    push eax
    call _soma
    add esp, 8
    mov [ebp - 12], eax
    ; print(...)
    ; String literal
    jmp .str_end_6
  .str_data_6 db , "Soma:", 0
  .str_end_6:
    lea eax, [.str_data_6]
    push eax
    push fmt_str
    call _printf
    add esp, 8
    ; print(...)
    ; Carregar variavel 's'
    mov eax, [ebp - 12]
    push eax
    push fmt_int
    call _printf
    add esp, 8
    ; Declaracao de variavel 'm'
    sub esp, 4
    ; Chamada de funcao 'maior'
    ; Carregar variavel 'b'
    mov eax, [ebp - 8]
    push eax
    ; Carregar variavel 'a'
    mov eax, [ebp - 4]
    push eax
    call _maior
    add esp, 8
    mov [ebp - 16], eax
    ; print(...)
    ; String literal
    jmp .str_end_7
  .str_data_7 db , "Maior:", 0
  .str_end_7:
    lea eax, [.str_data_7]
    push eax
    push fmt_str
    call _printf
    add esp, 8
    ; print(...)
    ; Carregar variavel 'm'
    mov eax, [ebp - 16]
    push eax
    push fmt_int
    call _printf
    add esp, 8
    ; Declaracao de variavel 'pot'
    sub esp, 4
    ; Chamada de funcao 'potencia'
    ; Literal inteiro: 8
    mov eax, 8
    push eax
    ; Literal inteiro: 2
    mov eax, 2
    push eax
    call _potencia
    add esp, 8
    mov [ebp - 20], eax
    ; print(...)
    ; String literal
    jmp .str_end_8
  .str_data_8 db , "2 elevado a 8 =", 0
  .str_end_8:
    lea eax, [.str_data_8]
    push eax
    push fmt_str
    call _printf
    add esp, 8
    ; print(...)
    ; Carregar variavel 'pot'
    mov eax, [ebp - 20]
    push eax
    push fmt_int
    call _printf
    add esp, 8
    ; if
    ; Chamada de funcao 'ehPositivo'
    ; Carregar variavel 'a'
    mov eax, [ebp - 4]
    push eax
    call _ehPositivo
    add esp, 4
    cmp eax, 0
    je .else_9
    ; print(...)
    ; String literal
    jmp .str_end_10
  .str_data_10 db , "Primeiro numero e positivo", 0
  .str_end_10:
    lea eax, [.str_data_10]
    push eax
    push fmt_str
    call _printf
    add esp, 8
    jmp .endif_9
.else_9:
    ; print(...)
    ; String literal
    jmp .str_end_11
  .str_data_11 db , "Primeiro numero nao e positivo", 0
  .str_end_11:
    lea eax, [.str_data_11]
    push eax
    push fmt_str
    call _printf
    add esp, 8
.endif_9:
    ; Declaracao de variavel 'resultado'
    sub esp, 4
    ; Operacao binaria '*'
    ; Chamada de funcao 'soma'
    ; Carregar variavel 'b'
    mov eax, [ebp - 8]
    push eax
    ; Carregar variavel 'a'
    mov eax, [ebp - 4]
    push eax
    call _soma
    add esp, 8
    push eax
    ; Chamada de funcao 'maior'
    ; Carregar variavel 'b'
    mov eax, [ebp - 8]
    push eax
    ; Carregar variavel 'a'
    mov eax, [ebp - 4]
    push eax
    call _maior
    add esp, 8
    mov ebx, eax
    pop eax
    imul eax, ebx
    mov [ebp - 24], eax
    ; print(...)
    ; String literal
    jmp .str_end_12
  .str_data_12 db , "soma(a,b) * maior(a,b) =", 0
  .str_end_12:
    lea eax, [.str_data_12]
    push eax
    push fmt_str
    call _printf
    add esp, 8
    ; print(...)
    ; Carregar variavel 'resultado'
    mov eax, [ebp - 24]
    push eax
    push fmt_int
    call _printf
    add esp, 8

    ; Encerramento do programa
    xor eax, eax
    mov esp, ebp
    pop ebp
    ret
