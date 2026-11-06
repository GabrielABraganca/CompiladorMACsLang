/*
 * =============================================================================
 * analisadorSintatico.h - Cabeçalho do Analisador Sintático da MACSLang
 * =============================================================================
 *
 * O ANALISADOR SINTÁTICO (também chamado de "parser") é a segunda fase do
 * compilador. Ele recebe os tokens produzidos pelo analisador léxico e
 * verifica se eles formam estruturas gramaticalmente corretas.
 *
 * O resultado desta fase é uma ÁRVORE SINTÁTICA ABSTRATA (AST - Abstract
 * Syntax Tree), que representa a estrutura hierárquica do programa.
 *
 * Exemplo:
 *   Código: "var x: int = 10 + 5;"
 *   AST:
 *     DeclaracaoVar
 *       └── nome: "x"
 *       └── tipo: int
 *       └── valor:
 *             Operacao(+)
 *               ├── esquerda: Numero(10)
 *               └── direita:  Numero(5)
 *
 * =============================================================================
 */

#ifndef ANALISADOR_SINTATICO_H
#define ANALISADOR_SINTATICO_H

#include "analisadorLexico.h"

/* Número máximo de filhos que um nó da AST pode ter */
#define MAX_FILHOS 32

/* Número máximo de tokens que podem ser armazenados */
#define MAX_TOKENS 8192

/*
 * =============================================================================
 * TIPOS DE NÓS DA ÁRVORE SINTÁTICA ABSTRATA (AST)
 * =============================================================================
 *
 * Cada constante representa um tipo diferente de nó na árvore.
 */
typedef enum {
    NO_PROGRAMA,        /* Raiz da árvore: o programa completo */
    NO_DECL_VAR,        /* Declaração de variável: var x: int = 10; */
    NO_DECL_FUNC,       /* Declaração de função: func nome(...): tipo { } */
    NO_BLOCO,           /* Bloco de código entre chaves: { ... } */
    NO_ATRIBUICAO,      /* Atribuição: x = expressão; */
    NO_IF,              /* Condicional: if (...) { } else { } */
    NO_WHILE,           /* Laço while: while (...) { } */
    NO_FOR,             /* Laço for: for (...;...;...) { } */
    NO_RETURN,          /* Retorno: return expressão; */
    NO_PRINT,           /* Impressão: print(expressão); */
    NO_INPUT,           /* Entrada: input(variável); */
    NO_CHAMADA_FUNC,    /* Chamada de função: nome(args...) */
    NO_OPERACAO_BIN,    /* Operação binária: a + b, a == b, etc. */
    NO_OPERACAO_UN,     /* Operação unária: !x, -x */
    NO_NUMERO_INT,      /* Literal inteiro: 42 */
    NO_NUMERO_FLOAT,    /* Literal float: 3.14 */
    NO_LITERAL_CHAR,    /* Literal char: 'a' */
    NO_LITERAL_STR,     /* Literal string: "hello" */
    NO_BOOLEANO,        /* Literal booleano: true, false */
    NO_IDENTIFICADOR,   /* Nome de variável: x, minhaVar */
    NO_PARAMETRO,       /* Parâmetro de função: nome: tipo */
    NO_TIPO             /* Tipo de dado: int, float, char, bool, string */
} TipoNo;

/*
 * =============================================================================
 * ESTRUTURA DE UM NÓ DA AST
 * =============================================================================
 *
 * Cada nó da árvore representa um elemento sintático do programa.
 * Um nó pode ter filhos, formando a estrutura hierárquica da árvore.
 */
typedef struct No {
    TipoNo tipo;                   /* Qual tipo de nó é este? */
    char   valor[MAX_LEXEMA];      /* Valor textual (nome da variável, operador, etc.) */
    int    linha;                  /* Linha no código fonte (para erros) */
    TipoToken tipoToken;           /* Tipo do token associado (para operadores) */

    /* Filhos do nó (sub-expressões, sub-declarações, etc.) */
    struct No *filhos[MAX_FILHOS]; /* Array de ponteiros para filhos */
    int        numFilhos;          /* Quantos filhos este nó tem */
} No;

/*
 * =============================================================================
 * ESTRUTURA DO ANALISADOR SINTÁTICO
 * =============================================================================
 */
typedef struct {
    Token tokens[MAX_TOKENS]; /* Array com todos os tokens do programa */
    int   numTokens;          /* Total de tokens */
    int   posicao;            /* Posição atual na leitura dos tokens */
    int   erros;              /* Contador de erros sintáticos encontrados */
} AnalisadorSintatico;

/* =============================================================================
 * FUNÇÕES PÚBLICAS DO ANALISADOR SINTÁTICO
 * =============================================================================
 */

/* Inicializa o analisador com os tokens gerados pelo léxico */
void iniciarSintatico(AnalisadorSintatico *as, AnalisadorLexico *al);

/* Analisa o programa completo e retorna a AST */
No *analisarPrograma(AnalisadorSintatico *as);

/* Libera toda a memória alocada para a AST */
void liberarAST(No *raiz);

/* Imprime a AST de forma indentada (para debug) */
void imprimirAST(No *raiz, int nivel);

/* Cria um novo nó da AST */
No *criarNo(TipoNo tipo, const char *valor, int linha);

/* Adiciona um filho a um nó */
void adicionarFilho(No *pai, No *filho);

#endif /* ANALISADOR_SINTATICO_H */
