/*
 * =============================================================================
 * geradorCodigo.c - Implementação do Gerador de Código da MACSLang
 * =============================================================================
 *
 * Gera código Assembly NASM x86 para Windows a partir da AST.
 *
 * REGISTRADORES UTILIZADOS:
 *   EAX - acumulador principal (resultados de expressões, retorno de funções)
 *   EBX - acumulador auxiliar (segundo operando de operações binárias)
 *   ECX - contador (laços)
 *   ESP - stack pointer (topo da pilha)
 *   EBP - base pointer (base do frame da função atual)
 *
 * SEÇÕES DO ARQUIVO .ASM GERADO:
 *   section .data   - strings e floats (constantes)
 *   section .bss    - variáveis globais não inicializadas
 *   section .text   - código executável
 *
 * =============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "geradorCodigo.h"

/* Protótipos internos */
static void gerarNo(GeradorCodigo *gen, No *no);
static void gerarExpressao(GeradorCodigo *gen, No *no);
static void gerarDeclFunc(GeradorCodigo *gen, No *no);
static void gerarDeclVar(GeradorCodigo *gen, No *no);
static void gerarAtribuicao(GeradorCodigo *gen, No *no);
static void gerarIf(GeradorCodigo *gen, No *no);
static void gerarWhile(GeradorCodigo *gen, No *no);
static void gerarFor(GeradorCodigo *gen, No *no);
static void gerarReturn(GeradorCodigo *gen, No *no);
static void gerarPrint(GeradorCodigo *gen, No *no);
static void gerarInput(GeradorCodigo *gen, No *no);
static void gerarChamadaFunc(GeradorCodigo *gen, No *no);
static void gerarOperacaoBin(GeradorCodigo *gen, No *no);
static void gerarBloco(GeradorCodigo *gen, No *no);
static int  novoLabel(GeradorCodigo *gen);
static void varreduraFuncoes(GeradorCodigo *gen, No *raiz);

/* =============================================================================
 * FUNÇÕES AUXILIARES
 * =============================================================================
 */

/*
 * Gera um número de label único e incrementa o contador.
 * Usado para criar labels de salto únicos (if_else_42, while_end_7, etc.)
 */
static int novoLabel(GeradorCodigo *gen) {
    return gen->contadorLabel++;
}

/*
 * Emite uma linha de assembly com indentação.
 * Facilita a geração e leitura do código gerado.
 */
static void emitir(GeradorCodigo *gen, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(gen->saida, "    "); /* Indentação de 4 espaços */
    vfprintf(gen->saida, fmt, args);
    fprintf(gen->saida, "\n");
    va_end(args);
}

/*
 * Emite um label (sem indentação).
 */
static void emitirLabel(GeradorCodigo *gen, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(gen->saida, fmt, args);
    fprintf(gen->saida, ":\n");
    va_end(args);
}

/*
 * Emite um comentário no assembly (para facilitar debug do .asm gerado).
 */
static void emitirComentario(GeradorCodigo *gen, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(gen->saida, "    ; ");
    vfprintf(gen->saida, fmt, args);
    fprintf(gen->saida, "\n");
    va_end(args);
}

/*
 * Busca o offset de uma variável local no frame da função.
 * Variáveis locais são armazenadas em [ebp - offset].
 * Retorna o offset ou -1 se for uma variável global.
 */
static int buscarOffsetVar(GeradorCodigo *gen, const char *nome) {
    Simbolo *s = buscarSimbolo(gen->tabela, nome);
    if (s && s->enderecoAsm != 0) {
        return s->enderecoAsm;
    }
    return -1; /* Não encontrado como local */
}

/*
 * Escapa uma string para uso em NASM (trata caracteres especiais).
 */
static void escaparString(FILE *f, const char *s) {
    int i = 0;
    int emString = 0;

    while (s[i]) {
        if (s[i] == '\n') {
            if (emString) { fprintf(f, "\""); emString = 0; }
            fprintf(f, ", 10");
        } else if (s[i] == '\t') {
            if (emString) { fprintf(f, "\""); emString = 0; }
            fprintf(f, ", 9");
        } else {
            if (!emString) { fprintf(f, ", \""); emString = 1; }
            fputc(s[i], f);
        }
        i++;
    }
    if (emString) fprintf(f, "\"");
}

/* =============================================================================
 * VARREDURA PRÉVIA (Pre-scan)
 * =============================================================================
 *
 * Antes de gerar o código principal, varremos a AST para saber:
 * - Quais funções existem (para declarar no início)
 * - Se o programa usa print/input (para incluir as funções de I/O)
 */
static void varreduraFuncoes(GeradorCodigo *gen, No *raiz) {
    if (!raiz) return;

    if (raiz->tipo == NO_PRINT) gen->temPrint = 1;
    if (raiz->tipo == NO_INPUT) gen->temInput = 1;
    if (raiz->tipo == NO_NUMERO_FLOAT) gen->temFloat = 1;

    for (int i = 0; i < raiz->numFilhos; i++) {
        varreduraFuncoes(gen, raiz->filhos[i]);
    }
}

/* =============================================================================
 * INICIALIZAÇÃO
 * =============================================================================
 */

void iniciarGerador(GeradorCodigo *gen, FILE *saida, TabelaSimbolos *tabela) {
    gen->saida          = saida;
    gen->tabela         = tabela;
    gen->contadorLabel  = 0;
    gen->offsetAtual    = 0;
    gen->funcaoAtual[0] = '\0';
    gen->temPrint       = 0;
    gen->temInput       = 0;
    gen->temFloat       = 0;
}

/* =============================================================================
 * GERAÇÃO DO CABEÇALHO DO ARQUIVO .ASM
 * =============================================================================
 */

/*
 * Gera a seção de dados (.data): strings de formato para printf/scanf,
 * e constantes de string usadas no programa.
 */
static void gerarSecaoData(GeradorCodigo *gen, No *raiz) {
    fprintf(gen->saida, "section .data\n");

    /* Formato para printf de inteiro */
    if (gen->temPrint) {
        fprintf(gen->saida, "    fmt_int    db \"%%d\", 10, 0\n");
        fprintf(gen->saida, "    fmt_float  db \"%%f\", 10, 0\n");
        fprintf(gen->saida, "    fmt_str    db \"%%s\", 10, 0\n");
        fprintf(gen->saida, "    fmt_char   db \"%%c\", 10, 0\n");
    }
    /* Formato para scanf */
    if (gen->temInput) {
        fprintf(gen->saida, "    fmt_in_int   db \"%%d\", 0\n");
        fprintf(gen->saida, "    fmt_in_float db \"%%f\", 0\n");
        fprintf(gen->saida, "    fmt_in_str   db \"%%s\", 0\n");
    }

    /* (Simplificação: strings inline são geradas diretamente na seção .text) */

    fprintf(gen->saida, "\n");
}

/*
 * Gera o cabeçalho geral do arquivo .asm.
 */
static void gerarCabecalho(GeradorCodigo *gen) {
    fprintf(gen->saida,
        "; =============================================================\n"
        "; Codigo gerado automaticamente pelo Compilador MACSLang\n"
        "; Arquitetura: x86 (32 bits) | Sistema: Windows\n"
        "; Montador:    NASM\n"
        "; =============================================================\n"
        "\n"
        "; Para compilar e linkar (Windows):\n"
        ";   nasm -f win32 programa.asm -o programa.obj\n"
        ";   gcc -m32 programa.obj -o programa.exe\n"
        "\n"
        "global _main\n"
        "extern _printf\n"
        "extern _scanf\n"
        "\n"
    );
}

/* =============================================================================
 * GERAÇÃO DE EXPRESSÕES
 * =============================================================================
 *
 * Estratégia: o resultado de cada expressão fica em EAX após a geração.
 * Para operações binárias, salvamos EAX na pilha, calculamos o lado direito,
 * depois recuperamos e operamos.
 */

static void gerarExpressao(GeradorCodigo *gen, No *no) {
    if (!no) return;

    switch (no->tipo) {

        /* ----------------------------------------------------------------
         * LITERAIS INTEIROS: coloca o valor diretamente em EAX
         * ---------------------------------------------------------------- */
        case NO_NUMERO_INT:
            emitirComentario(gen, "Literal inteiro: %s", no->valor);
            emitir(gen, "mov eax, %s", no->valor);
            break;

        /* ----------------------------------------------------------------
         * LITERAIS BOOLEANOS: true=1, false=0
         * ---------------------------------------------------------------- */
        case NO_BOOLEANO:
            emitirComentario(gen, "Booleano: %s", no->valor);
            emitir(gen, "mov eax, %s",
                   strcmp(no->valor, "true") == 0 ? "1" : "0");
            break;

        /* ----------------------------------------------------------------
         * LITERAL STRING: coloca ponteiro para a string em EAX
         * Para strings, geramos uma constante .data inline
         * ---------------------------------------------------------------- */
        case NO_LITERAL_STR: {
            int lbl = novoLabel(gen);
            /* A string será declarada na seção .data de forma inline via salto */
            /* Estratégia: colocamos o endereço direto */
            emitirComentario(gen, "String literal");
            fprintf(gen->saida, "    jmp .str_end_%d\n", lbl);
            fprintf(gen->saida, "  .str_data_%d db ", lbl);
            escaparString(gen->saida, no->valor);
            fprintf(gen->saida, ", 0\n");
            fprintf(gen->saida, "  .str_end_%d:\n", lbl);
            fprintf(gen->saida, "    lea eax, [.str_data_%d]\n", lbl);
            break;
        }

        /* ----------------------------------------------------------------
         * LITERAL CHAR: coloca valor ASCII em EAX
         * ---------------------------------------------------------------- */
        case NO_LITERAL_CHAR:
            emitirComentario(gen, "Char literal: '%s'", no->valor);
            emitir(gen, "mov eax, %d", (unsigned char)no->valor[0]);
            break;

        /* ----------------------------------------------------------------
         * IDENTIFICADOR: carrega variável da memória para EAX
         * ---------------------------------------------------------------- */
        case NO_IDENTIFICADOR: {
            emitirComentario(gen, "Carregar variavel '%s'", no->valor);
            int offset = buscarOffsetVar(gen, no->valor);
            if (offset > 0) {
                /* Variável local: [ebp - offset] */
                emitir(gen, "mov eax, [ebp - %d]", offset);
            } else {
                /* Variável global: acesso direto pelo nome */
                emitir(gen, "mov eax, [_%s]", no->valor);
            }
            break;
        }

        /* ----------------------------------------------------------------
         * CHAMADA DE FUNÇÃO: gera o código de chamada
         * ---------------------------------------------------------------- */
        case NO_CHAMADA_FUNC:
            gerarChamadaFunc(gen, no);
            break;

        /* ----------------------------------------------------------------
         * OPERAÇÃO BINÁRIA: gera o código para a operação
         * ---------------------------------------------------------------- */
        case NO_OPERACAO_BIN:
            gerarOperacaoBin(gen, no);
            break;

        /* ----------------------------------------------------------------
         * OPERAÇÃO UNÁRIA: nega ou inverte
         * ---------------------------------------------------------------- */
        case NO_OPERACAO_UN:
            emitirComentario(gen, "Operacao unaria '%s'", no->valor);
            gerarExpressao(gen, no->filhos[0]); /* Resultado em EAX */
            if (no->tipoToken == TOKEN_MENOS) {
                emitir(gen, "neg eax");   /* -EAX */
            } else if (no->tipoToken == TOKEN_NAO) {
                emitir(gen, "xor eax, 1"); /* Inverte bit 0 (true/false) */
            }
            break;

        default:
            break;
    }
}

/*
 * Gera código para uma operação binária.
 * Padrão: calcula esquerda → empilha → calcula direita → opera → resultado em EAX
 */
static void gerarOperacaoBin(GeradorCodigo *gen, No *no) {
    emitirComentario(gen, "Operacao binaria '%s'", no->valor);

    /* 1. Calcula o operando esquerdo → resultado em EAX */
    gerarExpressao(gen, no->filhos[0]);

    /* 2. Empilha EAX para não perder durante cálculo do lado direito */
    emitir(gen, "push eax");

    /* 3. Calcula o operando direito → resultado em EAX */
    gerarExpressao(gen, no->filhos[1]);

    /* 4. EBX = EAX (direito), EAX = topo da pilha (esquerdo) */
    emitir(gen, "mov ebx, eax");   /* ebx = direito */
    emitir(gen, "pop eax");        /* eax = esquerdo */

    /* 5. Executa a operação */
    switch (no->tipoToken) {
        case TOKEN_MAIS:
            emitir(gen, "add eax, ebx");
            break;

        case TOKEN_MENOS:
            emitir(gen, "sub eax, ebx");
            break;

        case TOKEN_MULT:
            emitir(gen, "imul eax, ebx");
            break;

        case TOKEN_DIV:
            emitir(gen, "cdq");          /* Estende EAX para EDX:EAX */
            emitir(gen, "idiv ebx");     /* EAX = EAX / EBX */
            break;

        case TOKEN_MOD:
            emitir(gen, "cdq");
            emitir(gen, "idiv ebx");
            emitir(gen, "mov eax, edx"); /* Resto fica em EDX */
            break;

        /* Operadores de comparação: resultado em EAX (1=verdade, 0=falso) */
        case TOKEN_IGUAL_IGUAL:
            emitir(gen, "cmp eax, ebx");
            emitir(gen, "sete al");        /* al = 1 se igual */
            emitir(gen, "movzx eax, al");  /* Estende para 32 bits */
            break;

        case TOKEN_DIFERENTE:
            emitir(gen, "cmp eax, ebx");
            emitir(gen, "setne al");
            emitir(gen, "movzx eax, al");
            break;

        case TOKEN_MENOR:
            emitir(gen, "cmp eax, ebx");
            emitir(gen, "setl al");
            emitir(gen, "movzx eax, al");
            break;

        case TOKEN_MAIOR:
            emitir(gen, "cmp eax, ebx");
            emitir(gen, "setg al");
            emitir(gen, "movzx eax, al");
            break;

        case TOKEN_MENOR_IGUAL:
            emitir(gen, "cmp eax, ebx");
            emitir(gen, "setle al");
            emitir(gen, "movzx eax, al");
            break;

        case TOKEN_MAIOR_IGUAL:
            emitir(gen, "cmp eax, ebx");
            emitir(gen, "setge al");
            emitir(gen, "movzx eax, al");
            break;

        /* Operadores lógicos */
        case TOKEN_E:
            emitir(gen, "and eax, ebx");  /* EAX = EAX AND EBX */
            emitir(gen, "setne al");       /* 1 se resultado != 0 */
            emitir(gen, "movzx eax, al");
            break;

        case TOKEN_OU:
            emitir(gen, "or eax, ebx");
            emitir(gen, "setne al");
            emitir(gen, "movzx eax, al");
            break;

        default:
            break;
    }
}

/* =============================================================================
 * GERAÇÃO DE DECLARAÇÕES E STATEMENTS
 * =============================================================================
 */

/*
 * Gera código para declaração de variável local.
 * Aloca espaço na pilha e inicializa se necessário.
 */
static void gerarDeclVar(GeradorCodigo *gen, No *no) {
    emitirComentario(gen, "Declaracao de variavel '%s'", no->valor);

    /* Aloca 4 bytes na pilha para a variável */
    gen->offsetAtual += 4;
    emitir(gen, "sub esp, 4");  /* Reserva espaço na pilha */

    /* Registra o offset na tabela de símbolos */
    Simbolo *s = buscarSimbolo(gen->tabela, no->valor);
    if (s) {
        s->enderecoAsm = gen->offsetAtual;
    }

    /* Inicializa se houver valor */
    if (no->numFilhos > 1) { /* filhos[0]=tipo, filhos[1]=valor */
        gerarExpressao(gen, no->filhos[1]);  /* Resultado em EAX */
        emitir(gen, "mov [ebp - %d], eax", gen->offsetAtual);
    } else {
        /* Inicializa com zero por padrão */
        emitir(gen, "mov dword [ebp - %d], 0", gen->offsetAtual);
    }
}

/*
 * Gera código para atribuição: x = expressão;
 */
static void gerarAtribuicao(GeradorCodigo *gen, No *no) {
    emitirComentario(gen, "Atribuicao a '%s'", no->valor);

    /* Calcula a expressão → resultado em EAX */
    gerarExpressao(gen, no->filhos[0]);

    /* Armazena EAX na variável */
    int offset = buscarOffsetVar(gen, no->valor);
    if (offset > 0) {
        emitir(gen, "mov [ebp - %d], eax", offset);
    } else {
        emitir(gen, "mov [_%s], eax", no->valor);
    }
}

/*
 * Gera código para chamada de função.
 * Convenção cdecl: argumentos na pilha (direita para esquerda).
 */
static void gerarChamadaFunc(GeradorCodigo *gen, No *no) {
    emitirComentario(gen, "Chamada de funcao '%s'", no->valor);

    /* Empilha argumentos da direita para a esquerda (cdecl) */
    for (int i = no->numFilhos - 1; i >= 0; i--) {
        gerarExpressao(gen, no->filhos[i]);
        emitir(gen, "push eax");
    }

    /* Chama a função */
    emitir(gen, "call _%s", no->valor);

    /* Limpa a pilha (cdecl: caller cleanup) */
    if (no->numFilhos > 0) {
        emitir(gen, "add esp, %d", no->numFilhos * 4);
    }

    /* Resultado está em EAX */
}

/*
 * Gera código para print().
 * Usa a função printf da libc do Windows.
 */
static void gerarPrint(GeradorCodigo *gen, No *no) {
    emitirComentario(gen, "print(...)");

    No *expr = no->filhos[0];

    /* Determina o tipo do argumento para usar o formato correto */
    if (expr->tipo == NO_LITERAL_STR) {
        /* String: usa fmt_str */
        gerarExpressao(gen, expr);  /* Ponteiro da string em EAX */
        emitir(gen, "push eax");
        emitir(gen, "push fmt_str");
        emitir(gen, "call _printf");
        emitir(gen, "add esp, 8");
    } else if (expr->tipo == NO_NUMERO_FLOAT) {
        gerarExpressao(gen, expr);
        emitir(gen, "push eax");
        emitir(gen, "push fmt_float");
        emitir(gen, "call _printf");
        emitir(gen, "add esp, 8");
    } else {
        /*
         * Para concatenação com + e tipos mistos, simplificamos:
         * Se for operação de soma com strings, geramos cada parte.
         * Caso contrário, imprimimos como inteiro.
         */
        if (expr->tipo == NO_OPERACAO_BIN && expr->tipoToken == TOKEN_MAIS) {
            /* Gera print das partes separadas (concatenação) */
            /* Parte esquerda */
            No *esq = expr->filhos[0];
            if (esq->tipo == NO_LITERAL_STR) {
                gerarExpressao(gen, esq);
                emitir(gen, "push eax");
                emitir(gen, "push fmt_str");
            } else {
                gerarExpressao(gen, esq);
                emitir(gen, "push eax");
                emitir(gen, "push fmt_int");
            }
            emitir(gen, "call _printf");
            emitir(gen, "add esp, 8");
            /* Parte direita */
            No *dir = expr->filhos[1];
            if (dir->tipo == NO_LITERAL_STR) {
                gerarExpressao(gen, dir);
                emitir(gen, "push eax");
                emitir(gen, "push fmt_str");
            } else {
                gerarExpressao(gen, dir);
                emitir(gen, "push eax");
                emitir(gen, "push fmt_int");
            }
            emitir(gen, "call _printf");
            emitir(gen, "add esp, 8");
            /* Newline extra */
            emitir(gen, "push 10");
            emitir(gen, "push fmt_char");
            emitir(gen, "call _printf");
            emitir(gen, "add esp, 8");
        } else {
            gerarExpressao(gen, expr);
            emitir(gen, "push eax");
            emitir(gen, "push fmt_int");
            emitir(gen, "call _printf");
            emitir(gen, "add esp, 8");
        }
    }
}

/*
 * Gera código para input().
 * Usa scanf da libc do Windows.
 */
static void gerarInput(GeradorCodigo *gen, No *no) {
    emitirComentario(gen, "input(...)");

    No *varNo = no->filhos[0];
    int offset = buscarOffsetVar(gen, varNo->valor);

    /* Passa o endereço da variável para scanf */
    if (offset > 0) {
        emitir(gen, "lea eax, [ebp - %d]", offset);
    } else {
        emitir(gen, "mov eax, _%s", varNo->valor);
    }

    emitir(gen, "push eax");
    emitir(gen, "push fmt_in_int");
    emitir(gen, "call _scanf");
    emitir(gen, "add esp, 8");
}

/*
 * Gera código para if-else.
 *
 * Assembly gerado:
 *   ; avalia condição
 *   cmp eax, 0
 *   je  .else_N      ; se falso, vai para else
 *   ; bloco then
 *   jmp .endif_N     ; pula o else
 * .else_N:
 *   ; bloco else (se existir)
 * .endif_N:
 */
static void gerarIf(GeradorCodigo *gen, No *no) {
    int lbl = novoLabel(gen);
    emitirComentario(gen, "if");

    /* Avalia condição → resultado em EAX */
    gerarExpressao(gen, no->filhos[0]);
    emitir(gen, "cmp eax, 0");
    emitir(gen, "je .else_%d", lbl);

    /* Bloco then */
    gerarBloco(gen, no->filhos[1]);
    emitir(gen, "jmp .endif_%d", lbl);

    /* Bloco else */
    emitirLabel(gen, ".else_%d", lbl);
    if (no->numFilhos > 2) {
        gerarBloco(gen, no->filhos[2]);
    }

    emitirLabel(gen, ".endif_%d", lbl);
}

/*
 * Gera código para while.
 *
 * Assembly gerado:
 * .while_start_N:
 *   ; avalia condição
 *   cmp eax, 0
 *   je  .while_end_N ; se falso, sai do laço
 *   ; corpo do laço
 *   jmp .while_start_N ; volta ao início
 * .while_end_N:
 */
static void gerarWhile(GeradorCodigo *gen, No *no) {
    int lbl = novoLabel(gen);
    emitirComentario(gen, "while");

    emitirLabel(gen, ".while_start_%d", lbl);

    /* Avalia condição */
    gerarExpressao(gen, no->filhos[0]);
    emitir(gen, "cmp eax, 0");
    emitir(gen, "je .while_end_%d", lbl);

    /* Corpo */
    gerarBloco(gen, no->filhos[1]);

    emitir(gen, "jmp .while_start_%d", lbl);
    emitirLabel(gen, ".while_end_%d", lbl);
}

/*
 * Gera código para for.
 *
 * Assembly:
 *   ; inicialização
 * .for_start_N:
 *   ; condição
 *   je  .for_end_N
 *   ; corpo
 *   ; incremento
 *   jmp .for_start_N
 * .for_end_N:
 */
static void gerarFor(GeradorCodigo *gen, No *no) {
    int lbl = novoLabel(gen);
    emitirComentario(gen, "for");

    /* Inicialização (filhos[0]) */
    gerarNo(gen, no->filhos[0]);

    emitirLabel(gen, ".for_start_%d", lbl);

    /* Condição (filhos[1]) */
    gerarExpressao(gen, no->filhos[1]);
    emitir(gen, "cmp eax, 0");
    emitir(gen, "je .for_end_%d", lbl);

    /* Corpo (filhos[3] = NO_BLOCO) */
    if (no->numFilhos > 3) {
        gerarBloco(gen, no->filhos[3]);
    }

    /* Incremento (filhos[2]) */
    gerarNo(gen, no->filhos[2]);

    emitir(gen, "jmp .for_start_%d", lbl);
    emitirLabel(gen, ".for_end_%d", lbl);
}

/*
 * Gera código para return.
 * O valor de retorno é colocado em EAX.
 */
static void gerarReturn(GeradorCodigo *gen, No *no) {
    emitirComentario(gen, "return");

    if (no->numFilhos > 0) {
        gerarExpressao(gen, no->filhos[0]);  /* Valor em EAX */
    } else {
        emitir(gen, "xor eax, eax");  /* Retorna 0 */
    }

    /* Restaura o frame e retorna */
    emitir(gen, "mov esp, ebp");
    emitir(gen, "pop ebp");
    emitir(gen, "ret");
}

/*
 * Gera código para um bloco {}.
 */
static void gerarBloco(GeradorCodigo *gen, No *no) {
    if (!no) return;
    int offsetAnterior = gen->offsetAtual;

    for (int i = 0; i < no->numFilhos; i++) {
        gerarNo(gen, no->filhos[i]);
    }

    /* Restaura variáveis locais do bloco */
    int variavelDoBloco = gen->offsetAtual - offsetAnterior;
    if (variavelDoBloco > 0) {
        emitir(gen, "add esp, %d  ; libera variaveis do bloco", variavelDoBloco);
        gen->offsetAtual = offsetAnterior;
    }
}

/*
 * Gera código para uma declaração de função.
 *
 * Prólogo padrão x86:
 *   push ebp
 *   mov  ebp, esp
 *   (alocação de variáveis locais)
 *
 * Epílogo padrão x86:
 *   mov  esp, ebp
 *   pop  ebp
 *   ret
 */
static void gerarDeclFunc(GeradorCodigo *gen, No *no) {
    int nFilhos = no->numFilhos;
    No *corpo   = no->filhos[nFilhos - 1]; /* Último filho = corpo */

    fprintf(gen->saida, "\n");
    fprintf(gen->saida, "; --- Funcao: %s ---\n", no->valor);
    fprintf(gen->saida, "_%s:\n", no->valor); /* Label da função */

    /* ---- PRÓLOGO ---- */
    emitirComentario(gen, "Prologo da funcao");
    emitir(gen, "push ebp");
    emitir(gen, "mov ebp, esp");

    /* Reserva espaço para variáveis locais
     * (simplificado: reservamos espaço fixo) */
    emitir(gen, "sub esp, %d  ; espaco para variaveis locais", MAX_VARIAVEIS_LOCAIS * 4);

    /* Salva contexto e offset anterior */
    int offsetAnterior = gen->offsetAtual;
    gen->offsetAtual   = 0;

    char funcAnterior[MAX_LEXEMA];
    strncpy(funcAnterior, gen->funcaoAtual, MAX_LEXEMA - 1);
    strncpy(gen->funcaoAtual, no->valor, MAX_LEXEMA - 1);

    /* Registra parâmetros: ficam acima de EBP (EBP+8, EBP+12, ...)
     * Na convenção cdecl, o primeiro arg fica em [ebp+8] */
    int numParams = nFilhos - 2;
    for (int i = 0; i < numParams; i++) {
        No *param = no->filhos[i];
        Simbolo *s = buscarSimbolo(gen->tabela, param->valor);
        if (s) {
            s->enderecoAsm = -(8 + i * 4); /* Endereço negativo = acima de EBP */
        }
    }

    /* ---- CORPO ---- */
    for (int i = 0; i < corpo->numFilhos; i++) {
        gerarNo(gen, corpo->filhos[i]);
    }

    /* ---- EPÍLOGO ---- */
    emitirComentario(gen, "Epilogo da funcao");
    emitir(gen, "xor eax, eax");  /* Retorno padrão = 0 */
    emitir(gen, "mov esp, ebp");
    emitir(gen, "pop ebp");
    emitir(gen, "ret");

    /* Restaura contexto */
    gen->offsetAtual = offsetAnterior;
    strncpy(gen->funcaoAtual, funcAnterior, MAX_LEXEMA - 1);
}

/*
 * Dispatcher: chama a função de geração correta para cada tipo de nó.
 */
static void gerarNo(GeradorCodigo *gen, No *no) {
    if (!no) return;

    switch (no->tipo) {
        case NO_DECL_VAR:    gerarDeclVar(gen, no);    break;
        case NO_DECL_FUNC:   gerarDeclFunc(gen, no);   break;
        case NO_ATRIBUICAO:  gerarAtribuicao(gen, no); break;
        case NO_IF:          gerarIf(gen, no);          break;
        case NO_WHILE:       gerarWhile(gen, no);       break;
        case NO_FOR:         gerarFor(gen, no);         break;
        case NO_RETURN:      gerarReturn(gen, no);      break;
        case NO_PRINT:       gerarPrint(gen, no);       break;
        case NO_INPUT:       gerarInput(gen, no);       break;
        case NO_CHAMADA_FUNC:gerarChamadaFunc(gen, no); break;
        case NO_BLOCO:       gerarBloco(gen, no);       break;
        default:
            /* Expressões usadas como statement */
            gerarExpressao(gen, no);
            break;
    }
}

/* =============================================================================
 * PONTO DE ENTRADA DO GERADOR
 * =============================================================================
 */

/*
 * Gera o código Assembly completo para o programa.
 *
 * Estrutura do arquivo .asm gerado:
 *
 * 1. Cabeçalho (comentários, declarações externas)
 * 2. section .data (strings de formato, constantes)
 * 3. section .text (código executável)
 *    a. Funções definidas pelo usuário
 *    b. _main (ponto de entrada)
 *       - Código global (fora de funções)
 *       - Chamada a ExitProcess
 */
void gerarCodigo(GeradorCodigo *gen, No *raiz) {
    /* Varredura prévia para saber se usa print/input */
    varreduraFuncoes(gen, raiz);

    /* 1. Cabeçalho */
    gerarCabecalho(gen);

    /* 2. Seção de dados */
    gerarSecaoData(gen, raiz);

    /* 3. Seção de código */
    fprintf(gen->saida, "section .text\n\n");

    /*
     * Primeira passagem: gera funções definidas pelo usuário.
     * As funções precisam ser geradas ANTES do _main para que
     * o _main possa chamá-las.
     */
    for (int i = 0; i < raiz->numFilhos; i++) {
        if (raiz->filhos[i]->tipo == NO_DECL_FUNC) {
            gerarDeclFunc(gen, raiz->filhos[i]);
        }
    }

    /*
     * Segunda passagem: gera o _main com o código global
     * (declarações de variáveis e statements fora de funções).
     */
    fprintf(gen->saida, "\n; === Ponto de entrada do programa ===\n");
    fprintf(gen->saida, "_main:\n");

    emitirComentario(gen, "Prologo do main");
    emitir(gen, "push ebp");
    emitir(gen, "mov ebp, esp");
    emitir(gen, "sub esp, %d  ; espaco para variaveis locais do main",
           MAX_VARIAVEIS_LOCAIS * 4);

    /* Gera o código global (declarações e statements fora de funções) */
    for (int i = 0; i < raiz->numFilhos; i++) {
        if (raiz->filhos[i]->tipo != NO_DECL_FUNC) {
            gerarNo(gen, raiz->filhos[i]);
        }
    }

    /* Encerramento do programa */
    fprintf(gen->saida, "\n");
    emitirComentario(gen, "Encerramento do programa");
    emitir(gen, "xor eax, eax");   /* Código de retorno 0 (sucesso) */
    emitir(gen, "mov esp, ebp");
    emitir(gen, "pop ebp");
    emitir(gen, "ret");
}
