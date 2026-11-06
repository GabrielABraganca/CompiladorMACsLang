/*
 * =============================================================================
 * analisadorSemantico.h - Cabeçalho do Analisador Semântico da MACSLang
 * =============================================================================
 *
 * O ANALISADOR SEMÂNTICO é a terceira fase do compilador. Ele percorre a AST
 * gerada pelo analisador sintático e verifica se o programa faz SENTIDO.
 *
 * Enquanto a sintaxe verifica a FORMA (gramática), a semântica verifica o
 * SIGNIFICADO. Por exemplo:
 *
 *   - Variáveis declaradas antes de usadas
 *   - Tipos compatíveis em operações (não somar string com int)
 *   - Funções chamadas com número correto de argumentos
 *   - Return com tipo compatível com o retorno da função
 *
 * TABELA DE SÍMBOLOS:
 * O analisador semântico mantém uma "tabela de símbolos" que armazena
 * informações sobre todas as variáveis e funções declaradas no programa.
 *
 * =============================================================================
 */

#ifndef ANALISADOR_SEMANTICO_H
#define ANALISADOR_SEMANTICO_H

#include "analisadorLexico.h"
#include "analisadorSintatico.h"

/* Número máximo de símbolos na tabela */
#define MAX_SIMBOLOS 512

/* Número máximo de escopos aninhados (blocos dentro de blocos) */
#define MAX_ESCOPOS 64

/* Número máximo de parâmetros de uma função */
#define MAX_PARAMS 16

/*
 * =============================================================================
 * TIPOS DE DADO INTERNOS DO COMPILADOR
 * =============================================================================
 *
 * Representação interna dos tipos da MACSLang.
 */
typedef enum {
    TIPO_INT,
    TIPO_FLOAT,
    TIPO_CHAR,
    TIPO_BOOL,
    TIPO_STRING,
    TIPO_VOID,    /* Para funções sem retorno (não usado diretamente na MACSLang) */
    TIPO_ERRO     /* Tipo inválido (resultado de erro semântico) */
} TipoDado;

/*
 * =============================================================================
 * SÍMBOLO (entrada na tabela de símbolos)
 * =============================================================================
 *
 * Cada símbolo representa uma variável ou função no programa.
 */
typedef struct {
    char     nome[MAX_LEXEMA]; /* Nome do símbolo */
    TipoDado tipo;             /* Tipo de dado */
    int      ehFuncao;         /* 1 se for função, 0 se for variável */
    int      numParams;        /* Número de parâmetros (só para funções) */
    TipoDado tiposParams[MAX_PARAMS]; /* Tipos dos parâmetros */
    int      escopo;           /* Nível de escopo onde foi declarado */
    int      linha;            /* Linha da declaração */
    int      enderecoAsm;      /* Endereço/offset no assembly (usado no gerador) */
} Simbolo;

/*
 * =============================================================================
 * TABELA DE SÍMBOLOS
 * =============================================================================
 *
 * Armazena todos os símbolos (variáveis e funções) do programa.
 * Implementada como um array simples com controle de escopo.
 */
typedef struct {
    Simbolo simbolos[MAX_SIMBOLOS]; /* Array de símbolos */
    int     numSimbolos;            /* Quantidade atual de símbolos */
    int     escopoAtual;            /* Nível de escopo atual */
} TabelaSimbolos;

/*
 * =============================================================================
 * ESTRUTURA DO ANALISADOR SEMÂNTICO
 * =============================================================================
 */
typedef struct {
    TabelaSimbolos tabela;    /* Tabela de símbolos */
    int    erros;             /* Contador de erros semânticos */
    char   funcaoAtual[MAX_LEXEMA]; /* Nome da função sendo analisada */
    TipoDado tipoRetornoAtual;      /* Tipo de retorno esperado */
} AnalisadorSemantico;

/* =============================================================================
 * FUNÇÕES PÚBLICAS DO ANALISADOR SEMÂNTICO
 * =============================================================================
 */

/* Inicializa o analisador semântico */
void iniciarSemantico(AnalisadorSemantico *sem);

/* Analisa a AST e preenche a tabela de símbolos */
void analisarSemantico(AnalisadorSemantico *sem, No *raiz);

/* Converte TipoDado para string legível */
const char *nomeTipo(TipoDado tipo);

/* Converte token de tipo para TipoDado interno */
TipoDado tokenParaTipo(TipoToken token);

/* Busca um símbolo na tabela */
Simbolo *buscarSimbolo(TabelaSimbolos *tabela, const char *nome);

/* Imprime a tabela de símbolos (para debug) */
void imprimirTabelaSimbolos(TabelaSimbolos *tabela);

#endif /* ANALISADOR_SEMANTICO_H */
