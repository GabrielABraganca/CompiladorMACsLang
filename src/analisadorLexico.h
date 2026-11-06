/*
 * =============================================================================
 * analisadorLexico.h - Cabeçalho do Analisador Léxico da MACSLang
 * =============================================================================
 *
 * O ANALISADOR LÉXICO (também chamado de "scanner" ou "tokenizador") é a
 * primeira fase do compilador. Ele lê o código-fonte caractere por caractere
 * e agrupa esses caracteres em unidades significativas chamadas TOKENS.
 *
 * Exemplo:
 *   Código fonte: "var x: int = 10;"
 *   Tokens gerados:
 *     [PALAVRA_CHAVE: "var"]
 *     [IDENTIFICADOR: "x"]
 *     [DOIS_PONTOS: ":"]
 *     [TIPO: "int"]
 *     [IGUAL: "="]
 *     [NUMERO_INT: "10"]
 *     [PONTO_VIRGULA: ";"]
 *
 * =============================================================================
 */

#ifndef ANALISADOR_LEXICO_H
#define ANALISADOR_LEXICO_H

/* Tamanho máximo que um lexema (valor do token) pode ter */
#define MAX_LEXEMA 256

/* Tamanho máximo do buffer de leitura do arquivo */
#define MAX_BUFFER 4096

/*
 * =============================================================================
 * ENUMERAÇÃO DOS TIPOS DE TOKEN
 * =============================================================================
 *
 * Cada constante representa um tipo diferente de token que a linguagem
 * MACSLang pode produzir.
 */
typedef enum {
    /* --- Palavras-chave da linguagem --- */
    TOKEN_VAR,          /* "var"    - declaração de variável */
    TOKEN_FUNC,         /* "func"   - declaração de função */
    TOKEN_RETURN,       /* "return" - retorno de função */
    TOKEN_IF,           /* "if"     - condicional */
    TOKEN_ELSE,         /* "else"   - alternativa do if */
    TOKEN_WHILE,        /* "while"  - laço while */
    TOKEN_FOR,          /* "for"    - laço for */
    TOKEN_PRINT,        /* "print"  - saída no console */
    TOKEN_INPUT,        /* "input"  - entrada do usuário */
    TOKEN_TRUE,         /* "true"   - booleano verdadeiro */
    TOKEN_FALSE,        /* "false"  - booleano falso */

    /* --- Tipos de dados --- */
    TOKEN_TIPO_INT,     /* "int"    - tipo inteiro */
    TOKEN_TIPO_FLOAT,   /* "float"  - tipo ponto flutuante */
    TOKEN_TIPO_CHAR,    /* "char"   - tipo caractere */
    TOKEN_TIPO_BOOL,    /* "bool"   - tipo booleano */
    TOKEN_TIPO_STRING,  /* "string" - tipo cadeia de caracteres */

    /* --- Literais (valores concretos escritos no código) --- */
    TOKEN_NUM_INT,      /* Número inteiro:  10, 42, -5 */
    TOKEN_NUM_FLOAT,    /* Número float:    3.14, 2.0 */
    TOKEN_LITERAL_CHAR, /* Caractere:       'a', 'Z' */
    TOKEN_LITERAL_STR,  /* String:          "hello world" */

    /* --- Identificador (nomes de variáveis e funções) --- */
    TOKEN_IDENTIFICADOR, /* Ex: minhaVariavel, fatorial, x */

    /* --- Operadores Aritméticos --- */
    TOKEN_MAIS,         /* + */
    TOKEN_MENOS,        /* - */
    TOKEN_MULT,         /* * */
    TOKEN_DIV,          /* / */
    TOKEN_MOD,          /* % */

    /* --- Operadores de Comparação --- */
    TOKEN_IGUAL_IGUAL,  /* == */
    TOKEN_DIFERENTE,    /* != */
    TOKEN_MENOR,        /* <  */
    TOKEN_MAIOR,        /* >  */
    TOKEN_MENOR_IGUAL,  /* <= */
    TOKEN_MAIOR_IGUAL,  /* >= */

    /* --- Operadores Lógicos --- */
    TOKEN_E,            /* && */
    TOKEN_OU,           /* || */
    TOKEN_NAO,          /* !  */

    /* --- Operador de Atribuição --- */
    TOKEN_IGUAL,        /* = */

    /* --- Delimitadores e Pontuação --- */
    TOKEN_PONTO_VIRGULA,  /* ; */
    TOKEN_DOIS_PONTOS,    /* : */
    TOKEN_VIRGULA,        /* , */
    TOKEN_ABRE_PAREN,     /* ( */
    TOKEN_FECHA_PAREN,    /* ) */
    TOKEN_ABRE_CHAVE,     /* { */
    TOKEN_FECHA_CHAVE,    /* } */

    /* --- Tokens especiais --- */
    TOKEN_EOF,          /* Fim do arquivo (End Of File) */
    TOKEN_ERRO          /* Token inválido (erro léxico) */
} TipoToken;

/*
 * =============================================================================
 * ESTRUTURA DO TOKEN
 * =============================================================================
 *
 * Um Token é a unidade básica que o analisador léxico produz.
 * Ele contém o tipo do token e o valor original (lexema).
 */
typedef struct {
    TipoToken tipo;           /* Qual tipo de token é este? */
    char lexema[MAX_LEXEMA];  /* O texto original do token (ex: "fatorial") */
    int linha;                /* Em qual linha do código fonte o token está */
} Token;

/*
 * =============================================================================
 * ESTRUTURA DO ANALISADOR LÉXICO
 * =============================================================================
 *
 * Mantém o estado interno do analisador durante a leitura do arquivo.
 */
typedef struct {
    char *fonte;      /* Ponteiro para o início do código fonte */
    int   posicao;    /* Posição atual de leitura no código fonte */
    int   linha;      /* Linha atual (para mensagens de erro) */
    int   tamanho;    /* Tamanho total do código fonte */
} AnalisadorLexico;

/* =============================================================================
 * FUNÇÕES PÚBLICAS DO ANALISADOR LÉXICO
 * =============================================================================
 */

/* Inicializa o analisador léxico com o conteúdo do arquivo fonte */
void iniciarLexico(AnalisadorLexico *al, char *fonte, int tamanho);

/* Retorna o próximo token do código fonte */
Token proximoToken(AnalisadorLexico *al);

/* Converte o tipo do token para uma string legível (para debug) */
const char *nomeToken(TipoToken tipo);

#endif /* ANALISADOR_LEXICO_H */
