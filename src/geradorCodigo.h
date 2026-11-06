/*
 * =============================================================================
 * geradorCodigo.h - Cabeçalho do Gerador de Código da MACSLang
 * =============================================================================
 *
 * O GERADOR DE CÓDIGO é a quarta e última fase do compilador. Ele percorre
 * a AST e produz código Assembly x86 (NASM) para Windows.
 *
 * ARQUITETURA ALVO: x86 (32 bits) para Windows
 * SINTAXE:          NASM (Netwide Assembler)
 *
 * ESTRATÉGIA DE GERAÇÃO:
 *
 * Usamos uma pilha (stack) como área de trabalho para expressões.
 * Para cada expressão:
 *   1. Avaliamos o lado esquerdo → empilhamos resultado
 *   2. Avaliamos o lado direito  → empilhamos resultado
 *   3. Executamos a operação     → desempilhamos, operamos, empilhamos resultado
 *
 * CONVENÇÃO DE CHAMADA (cdecl para Windows x86):
 *   - Argumentos passados pela pilha (da direita para esquerda)
 *   - EAX retorna o valor de retorno
 *   - Caller limpa a pilha após a chamada
 *
 * LAYOUT DE MEMÓRIA DAS VARIÁVEIS LOCAIS:
 *   - Armazenadas na pilha, referenciadas por [ebp - offset]
 *   - Exemplo: primeira variável em [ebp - 4], segunda em [ebp - 8], etc.
 *
 * =============================================================================
 */

#ifndef GERADOR_CODIGO_H
#define GERADOR_CODIGO_H

#include <stdio.h>
#include "analisadorSintatico.h"
#include "analisadorSemantico.h"

/* Número máximo de variáveis locais por função */
#define MAX_VARIAVEIS_LOCAIS 128

/* Número máximo de rótulos (labels) gerados */
#define MAX_ROTULOS 1024

/*
 * =============================================================================
 * ESTRUTURA DO GERADOR DE CÓDIGO
 * =============================================================================
 */
typedef struct {
    FILE            *saida;         /* Arquivo de saída (.asm) */
    TabelaSimbolos  *tabela;        /* Tabela de símbolos (do semântico) */
    int              contadorLabel; /* Contador para gerar labels únicos */
    int              offsetAtual;   /* Offset atual para variáveis locais */
    char             funcaoAtual[MAX_LEXEMA]; /* Função sendo gerada */
    int              temPrint;      /* Flag: o programa usa print? */
    int              temInput;      /* Flag: o programa usa input? */
    int              temFloat;      /* Flag: o programa usa floats? */
} GeradorCodigo;

/* =============================================================================
 * FUNÇÕES PÚBLICAS DO GERADOR DE CÓDIGO
 * =============================================================================
 */

/* Inicializa o gerador de código */
void iniciarGerador(GeradorCodigo *gen, FILE *saida, TabelaSimbolos *tabela);

/* Gera o código Assembly para a AST completa */
void gerarCodigo(GeradorCodigo *gen, No *raiz);

#endif /* GERADOR_CODIGO_H */
