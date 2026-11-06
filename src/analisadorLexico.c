/*
 * =============================================================================
 * analisadorLexico.c - Implementação do Analisador Léxico da MACSLang
 * =============================================================================
 *
 * Este arquivo implementa o analisador léxico (scanner) que transforma o
 * código-fonte em uma sequência de tokens.
 *
 * COMO FUNCIONA:
 *   1. Lê o código-fonte caractere por caractere
 *   2. Agrupa caracteres em lexemas (palavras, números, símbolos)
 *   3. Classifica cada lexema como um tipo de token
 *   4. Retorna um token por vez quando solicitado
 *
 * =============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>  /* isalpha, isdigit, isspace, etc. */
#include "analisadorLexico.h"

/*
 * =============================================================================
 * TABELA DE PALAVRAS-CHAVE
 * =============================================================================
 *
 * Esta tabela mapeia strings (texto) para tipos de token.
 * Quando o analisador encontra um identificador, ele verifica se é uma
 * palavra-chave consultando esta tabela.
 */
typedef struct {
    const char *palavra;  /* A palavra-chave em texto */
    TipoToken   tipo;     /* O tipo de token correspondente */
} PalavraChave;

/* Lista de todas as palavras reservadas da MACSLang */
static PalavraChave palavrasChave[] = {
    {"var",    TOKEN_VAR},
    {"func",   TOKEN_FUNC},
    {"return", TOKEN_RETURN},
    {"if",     TOKEN_IF},
    {"else",   TOKEN_ELSE},
    {"while",  TOKEN_WHILE},
    {"for",    TOKEN_FOR},
    {"print",  TOKEN_PRINT},
    {"input",  TOKEN_INPUT},
    {"true",   TOKEN_TRUE},
    {"false",  TOKEN_FALSE},
    {"int",    TOKEN_TIPO_INT},
    {"float",  TOKEN_TIPO_FLOAT},
    {"char",   TOKEN_TIPO_CHAR},
    {"bool",   TOKEN_TIPO_BOOL},
    {"string", TOKEN_TIPO_STRING},
    {NULL,     TOKEN_ERRO}  /* Sentinela: marca o fim da tabela */
};

/* =============================================================================
 * FUNÇÕES AUXILIARES INTERNAS (static = usadas só neste arquivo)
 * =============================================================================
 */

/*
 * Retorna o caractere na posição atual SEM avançar.
 * É como "olhar para frente" sem consumir o caractere.
 */
static char olharAtual(AnalisadorLexico *al) {
    if (al->posicao >= al->tamanho) return '\0'; /* '\0' = fim de string */
    return al->fonte[al->posicao];
}

/*
 * Retorna o próximo caractere (posição + 1) SEM avançar.
 * Usado para decidir tokens de dois caracteres como ==, !=, <=, >=, &&, ||
 */
static char olharProximo(AnalisadorLexico *al) {
    if (al->posicao + 1 >= al->tamanho) return '\0';
    return al->fonte[al->posicao + 1];
}

/*
 * Consome e retorna o caractere atual, avançando a posição.
 * Atualiza o contador de linha se encontrar '\n'.
 */
static char consumir(AnalisadorLexico *al) {
    char c = al->fonte[al->posicao];
    al->posicao++;
    if (c == '\n') al->linha++;
    return c;
}

/*
 * Pula espaços em branco e comentários.
 * Comentários em MACSLang: // (linha inteira)
 */
static void pularEspacosEComentarios(AnalisadorLexico *al) {
    while (al->posicao < al->tamanho) {
        char c = olharAtual(al);

        /* Pular espaços, tabs, quebras de linha */
        if (isspace(c)) {
            consumir(al);
        }
        /* Comentário de linha: // ... até o fim da linha */
        else if (c == '/' && olharProximo(al) == '/') {
            /* Consome o '//' */
            consumir(al);
            consumir(al);
            /* Continua consumindo até o fim da linha */
            while (al->posicao < al->tamanho && olharAtual(al) != '\n') {
                consumir(al);
            }
        }
        else {
            break; /* Encontrou algo útil, para de pular */
        }
    }
}

/*
 * Verifica se a string é uma palavra-chave.
 * Retorna o tipo do token se for palavra-chave, ou TOKEN_IDENTIFICADOR se não for.
 */
static TipoToken verificarPalavraChave(const char *texto) {
    int i = 0;
    while (palavrasChave[i].palavra != NULL) {
        if (strcmp(texto, palavrasChave[i].palavra) == 0) {
            return palavrasChave[i].tipo;
        }
        i++;
    }
    return TOKEN_IDENTIFICADOR; /* Não é palavra-chave, é identificador de usuário */
}

/*
 * Lê um número inteiro ou de ponto flutuante.
 * Exemplo: "123", "3.14", "0"
 */
static Token lerNumero(AnalisadorLexico *al) {
    Token tok;
    int len = 0;
    int ehFloat = 0;

    tok.linha = al->linha;

    /* Lê todos os dígitos */
    while (al->posicao < al->tamanho && isdigit(olharAtual(al))) {
        tok.lexema[len++] = consumir(al);
    }

    /* Verifica se tem ponto decimal (número float) */
    if (olharAtual(al) == '.' && isdigit(olharProximo(al))) {
        ehFloat = 1;
        tok.lexema[len++] = consumir(al); /* consume o '.' */
        /* Lê os dígitos após o ponto */
        while (al->posicao < al->tamanho && isdigit(olharAtual(al))) {
            tok.lexema[len++] = consumir(al);
        }
    }

    tok.lexema[len] = '\0';
    tok.tipo = ehFloat ? TOKEN_NUM_FLOAT : TOKEN_NUM_INT;
    return tok;
}

/*
 * Lê um identificador ou palavra-chave.
 * Identificadores começam com letra ou '_' e podem conter letras, dígitos e '_'.
 * Exemplo: "minhaVariavel", "x", "fatorial", "var", "if"
 */
static Token lerIdentificador(AnalisadorLexico *al) {
    Token tok;
    int len = 0;

    tok.linha = al->linha;

    /* Consome letras, dígitos e underscore */
    while (al->posicao < al->tamanho &&
           (isalnum(olharAtual(al)) || olharAtual(al) == '_')) {
        tok.lexema[len++] = consumir(al);
    }
    tok.lexema[len] = '\0';

    /* Verifica se é palavra-chave ou identificador do programador */
    tok.tipo = verificarPalavraChave(tok.lexema);
    return tok;
}

/*
 * Lê uma string entre aspas duplas.
 * Exemplo: "Olá, Mundo!", "Digite um número:"
 * Suporta sequências de escape: \n, \t, \\, \"
 */
static Token lerString(AnalisadorLexico *al) {
    Token tok;
    int len = 0;

    tok.linha = al->linha;
    tok.tipo  = TOKEN_LITERAL_STR;

    consumir(al); /* Consome a aspa de abertura " */

    while (al->posicao < al->tamanho && olharAtual(al) != '"') {
        char c = consumir(al);

        /* Trata sequências de escape */
        if (c == '\\') {
            char esc = consumir(al);
            switch (esc) {
                case 'n':  tok.lexema[len++] = '\n'; break;
                case 't':  tok.lexema[len++] = '\t'; break;
                case '\\': tok.lexema[len++] = '\\'; break;
                case '"':  tok.lexema[len++] = '"';  break;
                default:   tok.lexema[len++] = esc;  break;
            }
        } else {
            tok.lexema[len++] = c;
        }

        if (len >= MAX_LEXEMA - 1) break; /* Protege contra overflow */
    }

    if (olharAtual(al) == '"') {
        consumir(al); /* Consome a aspa de fechamento " */
    }

    tok.lexema[len] = '\0';
    return tok;
}

/*
 * Lê um caractere entre aspas simples.
 * Exemplo: 'a', 'Z', '\n'
 */
static Token lerChar(AnalisadorLexico *al) {
    Token tok;

    tok.linha = al->linha;
    tok.tipo  = TOKEN_LITERAL_CHAR;

    consumir(al); /* Consome ' de abertura */

    int len = 0;
    if (olharAtual(al) == '\\') {
        /* Sequência de escape dentro de char */
        consumir(al);
        char esc = consumir(al);
        tok.lexema[len++] = esc;
    } else {
        tok.lexema[len++] = consumir(al);
    }
    tok.lexema[len] = '\0';

    if (olharAtual(al) == '\'') {
        consumir(al); /* Consome ' de fechamento */
    }

    return tok;
}

/* =============================================================================
 * FUNÇÕES PÚBLICAS
 * =============================================================================
 */

/*
 * Inicializa o analisador léxico.
 * Deve ser chamada antes de usar proximoToken().
 *
 * Parâmetros:
 *   al     - ponteiro para a estrutura do analisador
 *   fonte  - string com o código-fonte completo
 *   tamanho - tamanho da string fonte
 */
void iniciarLexico(AnalisadorLexico *al, char *fonte, int tamanho) {
    al->fonte    = fonte;
    al->posicao  = 0;
    al->linha    = 1;  /* Linhas começam em 1 */
    al->tamanho  = tamanho;
}

/*
 * Retorna o próximo token do código fonte.
 *
 * Esta é a função principal do analisador léxico.
 * Cada chamada avança a posição e retorna um novo token.
 *
 * Retorna TOKEN_EOF quando o arquivo terminar.
 * Retorna TOKEN_ERRO se encontrar um caractere inválido.
 */
Token proximoToken(AnalisadorLexico *al) {
    Token tok;

    /* Pula espaços e comentários antes de ler o token */
    pularEspacosEComentarios(al);

    /* Verifica fim de arquivo */
    if (al->posicao >= al->tamanho) {
        tok.tipo = TOKEN_EOF;
        strcpy(tok.lexema, "EOF");
        tok.linha = al->linha;
        return tok;
    }

    char c = olharAtual(al);
    tok.linha = al->linha;

    /* -----------------------------------------------
     * NÚMEROS: começa com dígito
     * ----------------------------------------------- */
    if (isdigit(c)) {
        return lerNumero(al);
    }

    /* -----------------------------------------------
     * IDENTIFICADORES E PALAVRAS-CHAVE: começa com letra ou _
     * ----------------------------------------------- */
    if (isalpha(c) || c == '_') {
        return lerIdentificador(al);
    }

    /* -----------------------------------------------
     * STRINGS: começa com "
     * ----------------------------------------------- */
    if (c == '"') {
        return lerString(al);
    }

    /* -----------------------------------------------
     * CARACTERES: começa com '
     * ----------------------------------------------- */
    if (c == '\'') {
        return lerChar(al);
    }

    /* -----------------------------------------------
     * OPERADORES E DELIMITADORES: símbolos especiais
     * ----------------------------------------------- */
    consumir(al); /* Consome o caractere atual */

    switch (c) {
        /* Operadores que podem ter 1 ou 2 caracteres */
        case '=':
            if (olharAtual(al) == '=') {
                consumir(al);
                tok.tipo = TOKEN_IGUAL_IGUAL;
                strcpy(tok.lexema, "==");
            } else {
                tok.tipo = TOKEN_IGUAL;
                strcpy(tok.lexema, "=");
            }
            break;

        case '!':
            if (olharAtual(al) == '=') {
                consumir(al);
                tok.tipo = TOKEN_DIFERENTE;
                strcpy(tok.lexema, "!=");
            } else {
                tok.tipo = TOKEN_NAO;
                strcpy(tok.lexema, "!");
            }
            break;

        case '<':
            if (olharAtual(al) == '=') {
                consumir(al);
                tok.tipo = TOKEN_MENOR_IGUAL;
                strcpy(tok.lexema, "<=");
            } else {
                tok.tipo = TOKEN_MENOR;
                strcpy(tok.lexema, "<");
            }
            break;

        case '>':
            if (olharAtual(al) == '=') {
                consumir(al);
                tok.tipo = TOKEN_MAIOR_IGUAL;
                strcpy(tok.lexema, ">=");
            } else {
                tok.tipo = TOKEN_MAIOR;
                strcpy(tok.lexema, ">");
            }
            break;

        case '&':
            if (olharAtual(al) == '&') {
                consumir(al);
                tok.tipo = TOKEN_E;
                strcpy(tok.lexema, "&&");
            } else {
                tok.tipo = TOKEN_ERRO;
                strcpy(tok.lexema, "&");
            }
            break;

        case '|':
            if (olharAtual(al) == '|') {
                consumir(al);
                tok.tipo = TOKEN_OU;
                strcpy(tok.lexema, "||");
            } else {
                tok.tipo = TOKEN_ERRO;
                strcpy(tok.lexema, "|");
            }
            break;

        /* Operadores de um único caractere */
        case '+': tok.tipo = TOKEN_MAIS;         strcpy(tok.lexema, "+"); break;
        case '-': tok.tipo = TOKEN_MENOS;        strcpy(tok.lexema, "-"); break;
        case '*': tok.tipo = TOKEN_MULT;         strcpy(tok.lexema, "*"); break;
        case '/': tok.tipo = TOKEN_DIV;          strcpy(tok.lexema, "/"); break;
        case '%': tok.tipo = TOKEN_MOD;          strcpy(tok.lexema, "%"); break;

        /* Delimitadores */
        case ';': tok.tipo = TOKEN_PONTO_VIRGULA; strcpy(tok.lexema, ";"); break;
        case ':': tok.tipo = TOKEN_DOIS_PONTOS;   strcpy(tok.lexema, ":"); break;
        case ',': tok.tipo = TOKEN_VIRGULA;        strcpy(tok.lexema, ","); break;
        case '(': tok.tipo = TOKEN_ABRE_PAREN;    strcpy(tok.lexema, "("); break;
        case ')': tok.tipo = TOKEN_FECHA_PAREN;   strcpy(tok.lexema, ")"); break;
        case '{': tok.tipo = TOKEN_ABRE_CHAVE;    strcpy(tok.lexema, "{"); break;
        case '}': tok.tipo = TOKEN_FECHA_CHAVE;   strcpy(tok.lexema, "}"); break;

        /* Caractere não reconhecido = erro léxico */
        default:
            tok.tipo = TOKEN_ERRO;
            tok.lexema[0] = c;
            tok.lexema[1] = '\0';
            fprintf(stderr, "ERRO LEXICO [linha %d]: Caractere invalido '%c'\n",
                    tok.linha, c);
            break;
    }

    return tok;
}

/*
 * Converte o tipo do token para string legível.
 * Útil para imprimir mensagens de erro e debug.
 */
const char *nomeToken(TipoToken tipo) {
    switch (tipo) {
        case TOKEN_VAR:          return "VAR";
        case TOKEN_FUNC:         return "FUNC";
        case TOKEN_RETURN:       return "RETURN";
        case TOKEN_IF:           return "IF";
        case TOKEN_ELSE:         return "ELSE";
        case TOKEN_WHILE:        return "WHILE";
        case TOKEN_FOR:          return "FOR";
        case TOKEN_PRINT:        return "PRINT";
        case TOKEN_INPUT:        return "INPUT";
        case TOKEN_TRUE:         return "TRUE";
        case TOKEN_FALSE:        return "FALSE";
        case TOKEN_TIPO_INT:     return "TIPO_INT";
        case TOKEN_TIPO_FLOAT:   return "TIPO_FLOAT";
        case TOKEN_TIPO_CHAR:    return "TIPO_CHAR";
        case TOKEN_TIPO_BOOL:    return "TIPO_BOOL";
        case TOKEN_TIPO_STRING:  return "TIPO_STRING";
        case TOKEN_NUM_INT:      return "NUM_INT";
        case TOKEN_NUM_FLOAT:    return "NUM_FLOAT";
        case TOKEN_LITERAL_CHAR: return "LITERAL_CHAR";
        case TOKEN_LITERAL_STR:  return "LITERAL_STR";
        case TOKEN_IDENTIFICADOR:return "IDENTIFICADOR";
        case TOKEN_MAIS:         return "MAIS";
        case TOKEN_MENOS:        return "MENOS";
        case TOKEN_MULT:         return "MULT";
        case TOKEN_DIV:          return "DIV";
        case TOKEN_MOD:          return "MOD";
        case TOKEN_IGUAL_IGUAL:  return "IGUAL_IGUAL";
        case TOKEN_DIFERENTE:    return "DIFERENTE";
        case TOKEN_MENOR:        return "MENOR";
        case TOKEN_MAIOR:        return "MAIOR";
        case TOKEN_MENOR_IGUAL:  return "MENOR_IGUAL";
        case TOKEN_MAIOR_IGUAL:  return "MAIOR_IGUAL";
        case TOKEN_E:            return "E_LOGICO";
        case TOKEN_OU:           return "OU_LOGICO";
        case TOKEN_NAO:          return "NAO_LOGICO";
        case TOKEN_IGUAL:        return "IGUAL";
        case TOKEN_PONTO_VIRGULA:return "PONTO_VIRGULA";
        case TOKEN_DOIS_PONTOS:  return "DOIS_PONTOS";
        case TOKEN_VIRGULA:      return "VIRGULA";
        case TOKEN_ABRE_PAREN:   return "ABRE_PAREN";
        case TOKEN_FECHA_PAREN:  return "FECHA_PAREN";
        case TOKEN_ABRE_CHAVE:   return "ABRE_CHAVE";
        case TOKEN_FECHA_CHAVE:  return "FECHA_CHAVE";
        case TOKEN_EOF:          return "EOF";
        case TOKEN_ERRO:         return "ERRO";
        default:                 return "DESCONHECIDO";
    }
}
