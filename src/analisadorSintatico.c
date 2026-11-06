/*
 * =============================================================================
 * analisadorSintatico.c - Implementação do Analisador Sintático da MACSLang
 * =============================================================================
 *
 * ESTRATÉGIA DE ANÁLISE: Descida Recursiva (Recursive Descent)
 *
 * Para cada regra gramatical da linguagem, existe uma função correspondente.
 * As funções se chamam recursivamente para analisar estruturas aninhadas.
 *
 * GRAMÁTICA SIMPLIFICADA DA MACSLang:
 *
 *   programa       → declaracao*
 *   declaracao     → declVar | declFunc | statement
 *   declVar        → 'var' ID ':' tipo ('=' expressao)? ';'
 *   declFunc       → 'func' ID '(' parametros? ')' ':' tipo bloco
 *   statement      → atribuicao | if | while | for | return | print | input
 *   bloco          → '{' declaracao* '}'
 *   expressao      → ou (operadores de menor precedência primeiro)
 *   ou             → e ('||' e)*
 *   e              → comparacao ('&&' comparacao)*
 *   comparacao     → soma (('=='|'!='|'<'|'>'|'<='|'>=') soma)?
 *   soma           → termo (('+' | '-') termo)*
 *   termo          → fator (('*' | '/' | '%') fator)*
 *   fator          → '!' fator | '-' fator | primario
 *   primario       → NUM | STR | CHAR | BOOL | ID | ID'('args')' | '('expr')'
 *
 * =============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "analisadorSintatico.h"

/* =============================================================================
 * PROTÓTIPOS DAS FUNÇÕES INTERNAS
 * =============================================================================
 * Declaramos antes porque as funções se chamam mutuamente (recursão).
 */
static No *analisarDeclaracao(AnalisadorSintatico *as);
static No *analisarDeclVar(AnalisadorSintatico *as);
static No *analisarDeclFunc(AnalisadorSintatico *as);
static No *analisarBloco(AnalisadorSintatico *as);
static No *analisarStatement(AnalisadorSintatico *as);
static No *analisarIf(AnalisadorSintatico *as);
static No *analisarWhile(AnalisadorSintatico *as);
static No *analisarFor(AnalisadorSintatico *as);
static No *analisarReturn(AnalisadorSintatico *as);
static No *analisarPrint(AnalisadorSintatico *as);
static No *analisarInput(AnalisadorSintatico *as);
static No *analisarAtribuicao(AnalisadorSintatico *as);
static No *analisarExpressao(AnalisadorSintatico *as);
static No *analisarOu(AnalisadorSintatico *as);
static No *analisarE(AnalisadorSintatico *as);
static No *analisarComparacao(AnalisadorSintatico *as);
static No *analisarSoma(AnalisadorSintatico *as);
static No *analisarTermo(AnalisadorSintatico *as);
static No *analisarFator(AnalisadorSintatico *as);
static No *analisarPrimario(AnalisadorSintatico *as);
static No *analisarTipo(AnalisadorSintatico *as);

/* =============================================================================
 * FUNÇÕES DE GERENCIAMENTO DOS NÓS DA AST
 * =============================================================================
 */

/*
 * Cria um novo nó da AST com o tipo e valor fornecidos.
 * Aloca memória dinamicamente - deve ser liberada com liberarAST().
 */
No *criarNo(TipoNo tipo, const char *valor, int linha) {
    No *no = (No *)malloc(sizeof(No));
    if (!no) {
        fprintf(stderr, "ERRO: Falha ao alocar memoria para no da AST\n");
        exit(1);
    }
    no->tipo      = tipo;
    no->linha     = linha;
    no->numFilhos = 0;
    no->tipoToken = TOKEN_ERRO;

    if (valor) {
        strncpy(no->valor, valor, MAX_LEXEMA - 1);
        no->valor[MAX_LEXEMA - 1] = '\0';
    } else {
        no->valor[0] = '\0';
    }

    /* Inicializa todos os filhos como NULL */
    for (int i = 0; i < MAX_FILHOS; i++) {
        no->filhos[i] = NULL;
    }

    return no;
}

/*
 * Adiciona um filho a um nó pai.
 * Os filhos representam sub-expressões ou sub-declarações.
 */
void adicionarFilho(No *pai, No *filho) {
    if (!filho) return; /* Ignora filhos nulos */
    if (pai->numFilhos >= MAX_FILHOS) {
        fprintf(stderr, "ERRO: No com filhos demais (max %d)\n", MAX_FILHOS);
        return;
    }
    pai->filhos[pai->numFilhos++] = filho;
}

/*
 * Libera recursivamente toda a memória de uma árvore AST.
 */
void liberarAST(No *raiz) {
    if (!raiz) return;
    for (int i = 0; i < raiz->numFilhos; i++) {
        liberarAST(raiz->filhos[i]);
    }
    free(raiz);
}

/*
 * Imprime a AST de forma indentada para facilitar o debug.
 * 'nivel' controla a indentação.
 */
void imprimirAST(No *raiz, int nivel) {
    if (!raiz) return;

    /* Indentação com espaços */
    for (int i = 0; i < nivel * 2; i++) printf(" ");

    /* Nomes legíveis para cada tipo de nó */
    const char *nomes[] = {
        "PROGRAMA", "DECL_VAR", "DECL_FUNC", "BLOCO",
        "ATRIBUICAO", "IF", "WHILE", "FOR", "RETURN",
        "PRINT", "INPUT", "CHAMADA_FUNC", "OPERACAO_BIN",
        "OPERACAO_UN", "NUMERO_INT", "NUMERO_FLOAT",
        "LITERAL_CHAR", "LITERAL_STR", "BOOLEANO",
        "IDENTIFICADOR", "PARAMETRO", "TIPO"
    };

    printf("[%s] \"%s\" (linha %d)\n",
           nomes[raiz->tipo], raiz->valor, raiz->linha);

    /* Imprime os filhos recursivamente */
    for (int i = 0; i < raiz->numFilhos; i++) {
        imprimirAST(raiz->filhos[i], nivel + 1);
    }
}

/* =============================================================================
 * FUNÇÕES AUXILIARES PARA NAVEGAR NOS TOKENS
 * =============================================================================
 */

/*
 * Retorna o token atual sem avançar.
 */
static Token tokenAtual(AnalisadorSintatico *as) {
    if (as->posicao >= as->numTokens) {
        Token eof = {TOKEN_EOF, "EOF", 0};
        return eof;
    }
    return as->tokens[as->posicao];
}

/*
 * Avança para o próximo token e retorna o atual.
 */
static Token consumirToken(AnalisadorSintatico *as) {
    Token t = tokenAtual(as);
    if (as->posicao < as->numTokens) as->posicao++;
    return t;
}

/*
 * Verifica se o token atual é do tipo esperado.
 * Se sim, avança e retorna o token.
 * Se não, imprime erro e retorna token de erro.
 *
 * Esta função implementa o "casamento" (match) de tokens.
 */
static Token esperarToken(AnalisadorSintatico *as, TipoToken esperado) {
    Token atual = tokenAtual(as);

    if (atual.tipo == esperado) {
        return consumirToken(as);
    }

    /* Token não encontrado: registra o erro */
    fprintf(stderr,
        "ERRO SINTATICO [linha %d]: Esperado '%s', encontrado '%s' (\"%s\")\n",
        atual.linha, nomeToken(esperado), nomeToken(atual.tipo), atual.lexema);

    as->erros++;

    /* Retorna um token de erro mas NÃO avança (para tentar recuperação) */
    Token erro = {TOKEN_ERRO, "ERRO", atual.linha};
    return erro;
}

/*
 * Verifica se o token atual é de um dos tipos de tipo de dado.
 */
static int ehTipo(TipoToken tipo) {
    return tipo == TOKEN_TIPO_INT   ||
           tipo == TOKEN_TIPO_FLOAT ||
           tipo == TOKEN_TIPO_CHAR  ||
           tipo == TOKEN_TIPO_BOOL  ||
           tipo == TOKEN_TIPO_STRING;
}

/*
 * Verifica se o token atual é um operador de comparação.
 */
static int ehOperadorComparacao(TipoToken tipo) {
    return tipo == TOKEN_IGUAL_IGUAL ||
           tipo == TOKEN_DIFERENTE   ||
           tipo == TOKEN_MENOR       ||
           tipo == TOKEN_MAIOR       ||
           tipo == TOKEN_MENOR_IGUAL ||
           tipo == TOKEN_MAIOR_IGUAL;
}

/* =============================================================================
 * INICIALIZAÇÃO
 * =============================================================================
 */

/*
 * Inicializa o analisador sintático.
 * Lê todos os tokens do analisador léxico e os armazena em um array.
 *
 * Estratégia: lemos TODOS os tokens primeiro, depois os analisamos.
 * Isso facilita o "look-ahead" (olhar tokens futuros).
 */
void iniciarSintatico(AnalisadorSintatico *as, AnalisadorLexico *al) {
    as->numTokens = 0;
    as->posicao   = 0;
    as->erros     = 0;

    /* Lê todos os tokens até EOF */
    Token t;
    do {
        t = proximoToken(al);
        if (t.tipo != TOKEN_ERRO) {  /* Ignora tokens de erro léxico */
            as->tokens[as->numTokens++] = t;
        }
        if (as->numTokens >= MAX_TOKENS) {
            fprintf(stderr, "ERRO: Programa muito grande (max %d tokens)\n", MAX_TOKENS);
            break;
        }
    } while (t.tipo != TOKEN_EOF);
}

/* =============================================================================
 * ANÁLISE DO PROGRAMA (PONTO DE ENTRADA)
 * =============================================================================
 */

/*
 * Ponto de entrada da análise sintática.
 * Analisa o programa completo e retorna a raiz da AST.
 *
 * programa → declaracao*
 */
No *analisarPrograma(AnalisadorSintatico *as) {
    No *programa = criarNo(NO_PROGRAMA, "programa", 1);

    /* Analisa declarações até o fim do arquivo */
    while (tokenAtual(as).tipo != TOKEN_EOF) {
        No *decl = analisarDeclaracao(as);
        if (decl) {
            adicionarFilho(programa, decl);
        }
    }

    if (as->erros > 0) {
        fprintf(stderr, "\nTotal de erros sintaticos: %d\n", as->erros);
    }

    return programa;
}

/* =============================================================================
 * ANÁLISE DE DECLARAÇÕES
 * =============================================================================
 */

/*
 * Analisa uma declaração qualquer.
 * Verifica o token atual para decidir que tipo de declaração é.
 *
 * declaracao → declVar | declFunc | statement
 */
static No *analisarDeclaracao(AnalisadorSintatico *as) {
    Token atual = tokenAtual(as);

    switch (atual.tipo) {
        case TOKEN_VAR:  return analisarDeclVar(as);
        case TOKEN_FUNC: return analisarDeclFunc(as);
        default:         return analisarStatement(as);
    }
}

/*
 * Analisa uma declaração de variável.
 *
 * declVar → 'var' ID ':' tipo ('=' expressao)? ';'
 * Exemplo: var x: int = 10;
 *          var nome: string;
 */
static No *analisarDeclVar(AnalisadorSintatico *as) {
    int linha = tokenAtual(as).linha;

    esperarToken(as, TOKEN_VAR);         /* Consome 'var' */

    Token nomeTok = esperarToken(as, TOKEN_IDENTIFICADOR); /* Nome da variável */
    No *no = criarNo(NO_DECL_VAR, nomeTok.lexema, linha);

    esperarToken(as, TOKEN_DOIS_PONTOS); /* Consome ':' */

    No *tipo = analisarTipo(as);         /* Lê o tipo */
    adicionarFilho(no, tipo);

    /* Verifica se há inicialização com '=' */
    if (tokenAtual(as).tipo == TOKEN_IGUAL) {
        consumirToken(as);               /* Consome '=' */
        No *valor = analisarExpressao(as);
        adicionarFilho(no, valor);
    }

    esperarToken(as, TOKEN_PONTO_VIRGULA); /* Consome ';' */

    return no;
}

/*
 * Analisa uma declaração de função.
 *
 * declFunc → 'func' ID '(' parametros? ')' ':' tipo bloco
 * Exemplo:
 *   func soma(a: int, b: int): int {
 *       return a + b;
 *   }
 */
static No *analisarDeclFunc(AnalisadorSintatico *as) {
    int linha = tokenAtual(as).linha;

    esperarToken(as, TOKEN_FUNC);           /* Consome 'func' */

    Token nomeTok = esperarToken(as, TOKEN_IDENTIFICADOR);
    No *no = criarNo(NO_DECL_FUNC, nomeTok.lexema, linha);

    esperarToken(as, TOKEN_ABRE_PAREN);    /* Consome '(' */

    /* Analisa a lista de parâmetros */
    while (tokenAtual(as).tipo != TOKEN_FECHA_PAREN &&
           tokenAtual(as).tipo != TOKEN_EOF) {

        /* Cada parâmetro: nome: tipo */
        Token paramNome = esperarToken(as, TOKEN_IDENTIFICADOR);
        No *param = criarNo(NO_PARAMETRO, paramNome.lexema, paramNome.linha);

        esperarToken(as, TOKEN_DOIS_PONTOS);
        No *paramTipo = analisarTipo(as);
        adicionarFilho(param, paramTipo);
        adicionarFilho(no, param);

        /* Se houver vírgula, há mais parâmetros */
        if (tokenAtual(as).tipo == TOKEN_VIRGULA) {
            consumirToken(as);
        } else {
            break;
        }
    }

    esperarToken(as, TOKEN_FECHA_PAREN);   /* Consome ')' */
    esperarToken(as, TOKEN_DOIS_PONTOS);   /* Consome ':' */

    No *tipoRetorno = analisarTipo(as);    /* Tipo de retorno */
    adicionarFilho(no, tipoRetorno);

    No *corpo = analisarBloco(as);         /* Corpo da função */
    adicionarFilho(no, corpo);

    return no;
}

/*
 * Analisa um bloco de código entre chaves.
 *
 * bloco → '{' declaracao* '}'
 */
static No *analisarBloco(AnalisadorSintatico *as) {
    int linha = tokenAtual(as).linha;
    No *bloco = criarNo(NO_BLOCO, "bloco", linha);

    esperarToken(as, TOKEN_ABRE_CHAVE);    /* Consome '{' */

    /* Analisa declarações até fechar a chave */
    while (tokenAtual(as).tipo != TOKEN_FECHA_CHAVE &&
           tokenAtual(as).tipo != TOKEN_EOF) {
        No *decl = analisarDeclaracao(as);
        if (decl) adicionarFilho(bloco, decl);
    }

    esperarToken(as, TOKEN_FECHA_CHAVE);   /* Consome '}' */

    return bloco;
}

/* =============================================================================
 * ANÁLISE DE STATEMENTS (COMANDOS)
 * =============================================================================
 */

/*
 * Analisa um statement (comando) da linguagem.
 */
static No *analisarStatement(AnalisadorSintatico *as) {
    Token atual = tokenAtual(as);

    switch (atual.tipo) {
        case TOKEN_IF:     return analisarIf(as);
        case TOKEN_WHILE:  return analisarWhile(as);
        case TOKEN_FOR:    return analisarFor(as);
        case TOKEN_RETURN: return analisarReturn(as);
        case TOKEN_PRINT:  return analisarPrint(as);
        case TOKEN_INPUT:  return analisarInput(as);

        /* Identificador pode ser atribuição ou chamada de função */
        case TOKEN_IDENTIFICADOR:
            return analisarAtribuicao(as);

        default:
            fprintf(stderr,
                "ERRO SINTATICO [linha %d]: Statement invalido: '%s'\n",
                atual.linha, atual.lexema);
            as->erros++;
            consumirToken(as); /* Pula o token inválido para tentar continuar */
            return NULL;
    }
}

/*
 * Analisa uma estrutura if-else.
 *
 * if → 'if' '(' expressao ')' bloco ('else' bloco)?
 */
static No *analisarIf(AnalisadorSintatico *as) {
    int linha = tokenAtual(as).linha;
    No *no = criarNo(NO_IF, "if", linha);

    esperarToken(as, TOKEN_IF);            /* Consome 'if' */
    esperarToken(as, TOKEN_ABRE_PAREN);    /* Consome '(' */

    No *condicao = analisarExpressao(as);  /* Condição */
    adicionarFilho(no, condicao);

    esperarToken(as, TOKEN_FECHA_PAREN);   /* Consome ')' */

    No *blocoIf = analisarBloco(as);       /* Bloco then */
    adicionarFilho(no, blocoIf);

    /* Verifica se há else */
    if (tokenAtual(as).tipo == TOKEN_ELSE) {
        consumirToken(as);                 /* Consome 'else' */
        No *blocoElse = analisarBloco(as); /* Bloco else */
        adicionarFilho(no, blocoElse);
    }

    return no;
}

/*
 * Analisa um laço while.
 *
 * while → 'while' '(' expressao ')' bloco
 */
static No *analisarWhile(AnalisadorSintatico *as) {
    int linha = tokenAtual(as).linha;
    No *no = criarNo(NO_WHILE, "while", linha);

    esperarToken(as, TOKEN_WHILE);
    esperarToken(as, TOKEN_ABRE_PAREN);

    No *condicao = analisarExpressao(as);
    adicionarFilho(no, condicao);

    esperarToken(as, TOKEN_FECHA_PAREN);

    No *corpo = analisarBloco(as);
    adicionarFilho(no, corpo);

    return no;
}

/*
 * Analisa um laço for.
 *
 * for → 'for' '(' declVar|atrib ';' expressao ';' atrib ')' bloco
 * Exemplo: for (var i: int = 0; i < 10; i = i + 1) { }
 */
static No *analisarFor(AnalisadorSintatico *as) {
    int linha = tokenAtual(as).linha;
    No *no = criarNo(NO_FOR, "for", linha);

    esperarToken(as, TOKEN_FOR);
    esperarToken(as, TOKEN_ABRE_PAREN);

    /* Inicialização: pode ser var ou atribuição */
    No *init;
    if (tokenAtual(as).tipo == TOKEN_VAR) {
        init = analisarDeclVar(as);  /* 'var i: int = 0;' - já consome o ';' */
    } else {
        init = analisarAtribuicao(as);
    }
    adicionarFilho(no, init);

    /* Condição */
    No *condicao = analisarExpressao(as);
    adicionarFilho(no, condicao);
    esperarToken(as, TOKEN_PONTO_VIRGULA);

    /* Incremento: atribuição sem o ';' final */
    Token nomeVar = esperarToken(as, TOKEN_IDENTIFICADOR);
    No *incr = criarNo(NO_ATRIBUICAO, nomeVar.lexema, nomeVar.linha);
    esperarToken(as, TOKEN_IGUAL);
    No *exprIncr = analisarExpressao(as);
    adicionarFilho(incr, exprIncr);
    adicionarFilho(no, incr);

    esperarToken(as, TOKEN_FECHA_PAREN);

    No *corpo = analisarBloco(as);
    adicionarFilho(no, corpo);

    return no;
}

/*
 * Analisa um return.
 *
 * return → 'return' expressao ';'
 */
static No *analisarReturn(AnalisadorSintatico *as) {
    int linha = tokenAtual(as).linha;
    No *no = criarNo(NO_RETURN, "return", linha);

    esperarToken(as, TOKEN_RETURN);

    /* Pode não ter valor de retorno (return;) */
    if (tokenAtual(as).tipo != TOKEN_PONTO_VIRGULA) {
        No *expr = analisarExpressao(as);
        adicionarFilho(no, expr);
    }

    esperarToken(as, TOKEN_PONTO_VIRGULA);
    return no;
}

/*
 * Analisa um print.
 *
 * print → 'print' '(' expressao ')' ';'
 */
static No *analisarPrint(AnalisadorSintatico *as) {
    int linha = tokenAtual(as).linha;
    No *no = criarNo(NO_PRINT, "print", linha);

    esperarToken(as, TOKEN_PRINT);
    esperarToken(as, TOKEN_ABRE_PAREN);

    /* Pode ter múltiplas expressões separadas por + (concatenação) */
    No *expr = analisarExpressao(as);
    adicionarFilho(no, expr);

    esperarToken(as, TOKEN_FECHA_PAREN);
    esperarToken(as, TOKEN_PONTO_VIRGULA);

    return no;
}

/*
 * Analisa um input.
 *
 * input → 'input' '(' ID ')' ';'
 */
static No *analisarInput(AnalisadorSintatico *as) {
    int linha = tokenAtual(as).linha;
    No *no = criarNo(NO_INPUT, "input", linha);

    esperarToken(as, TOKEN_INPUT);
    esperarToken(as, TOKEN_ABRE_PAREN);

    Token varTok = esperarToken(as, TOKEN_IDENTIFICADOR);
    No *var = criarNo(NO_IDENTIFICADOR, varTok.lexema, varTok.linha);
    adicionarFilho(no, var);

    esperarToken(as, TOKEN_FECHA_PAREN);
    esperarToken(as, TOKEN_PONTO_VIRGULA);

    return no;
}

/*
 * Analisa uma atribuição ou chamada de função como statement.
 *
 * atribuicao → ID '=' expressao ';'
 *            | ID '(' args ')' ';'    (chamada como statement)
 */
static No *analisarAtribuicao(AnalisadorSintatico *as) {
    Token nomeTok = esperarToken(as, TOKEN_IDENTIFICADOR);

    /* Chamada de função como statement: nome(...); */
    if (tokenAtual(as).tipo == TOKEN_ABRE_PAREN) {
        No *chamada = criarNo(NO_CHAMADA_FUNC, nomeTok.lexema, nomeTok.linha);
        consumirToken(as); /* Consome '(' */

        /* Analisa argumentos */
        while (tokenAtual(as).tipo != TOKEN_FECHA_PAREN &&
               tokenAtual(as).tipo != TOKEN_EOF) {
            No *arg = analisarExpressao(as);
            adicionarFilho(chamada, arg);
            if (tokenAtual(as).tipo == TOKEN_VIRGULA) consumirToken(as);
            else break;
        }
        esperarToken(as, TOKEN_FECHA_PAREN);
        esperarToken(as, TOKEN_PONTO_VIRGULA);
        return chamada;
    }

    /* Atribuição: nome = expressão; */
    No *no = criarNo(NO_ATRIBUICAO, nomeTok.lexema, nomeTok.linha);
    esperarToken(as, TOKEN_IGUAL);
    No *expr = analisarExpressao(as);
    adicionarFilho(no, expr);
    esperarToken(as, TOKEN_PONTO_VIRGULA);

    return no;
}

/* =============================================================================
 * ANÁLISE DE EXPRESSÕES (com precedência de operadores)
 * =============================================================================
 *
 * A precedência é implementada pela hierarquia de funções:
 * expressao → ou → e → comparacao → soma → termo → fator → primario
 *
 * Quanto mais "fundo" na hierarquia, maior a precedência.
 * Por isso * tem precedência maior que +: `termo` chama `fator`,
 * e `soma` chama `termo`.
 */

/* expressao → ou */
static No *analisarExpressao(AnalisadorSintatico *as) {
    return analisarOu(as);
}

/* ou → e ('||' e)* */
static No *analisarOu(AnalisadorSintatico *as) {
    No *esq = analisarE(as);

    while (tokenAtual(as).tipo == TOKEN_OU) {
        Token op = consumirToken(as);
        No *no = criarNo(NO_OPERACAO_BIN, op.lexema, op.linha);
        no->tipoToken = TOKEN_OU;
        adicionarFilho(no, esq);
        adicionarFilho(no, analisarE(as));
        esq = no;
    }

    return esq;
}

/* e → comparacao ('&&' comparacao)* */
static No *analisarE(AnalisadorSintatico *as) {
    No *esq = analisarComparacao(as);

    while (tokenAtual(as).tipo == TOKEN_E) {
        Token op = consumirToken(as);
        No *no = criarNo(NO_OPERACAO_BIN, op.lexema, op.linha);
        no->tipoToken = TOKEN_E;
        adicionarFilho(no, esq);
        adicionarFilho(no, analisarComparacao(as));
        esq = no;
    }

    return esq;
}

/* comparacao → soma (('=='|'!='|'<'|'>'|'<='|'>=') soma)? */
static No *analisarComparacao(AnalisadorSintatico *as) {
    No *esq = analisarSoma(as);

    if (ehOperadorComparacao(tokenAtual(as).tipo)) {
        Token op = consumirToken(as);
        No *no = criarNo(NO_OPERACAO_BIN, op.lexema, op.linha);
        no->tipoToken = op.tipo;
        adicionarFilho(no, esq);
        adicionarFilho(no, analisarSoma(as));
        return no;
    }

    return esq;
}

/* soma → termo (('+' | '-') termo)* */
static No *analisarSoma(AnalisadorSintatico *as) {
    No *esq = analisarTermo(as);

    while (tokenAtual(as).tipo == TOKEN_MAIS ||
           tokenAtual(as).tipo == TOKEN_MENOS) {
        Token op = consumirToken(as);
        No *no = criarNo(NO_OPERACAO_BIN, op.lexema, op.linha);
        no->tipoToken = op.tipo;
        adicionarFilho(no, esq);
        adicionarFilho(no, analisarTermo(as));
        esq = no;
    }

    return esq;
}

/* termo → fator (('*' | '/' | '%') fator)* */
static No *analisarTermo(AnalisadorSintatico *as) {
    No *esq = analisarFator(as);

    while (tokenAtual(as).tipo == TOKEN_MULT ||
           tokenAtual(as).tipo == TOKEN_DIV  ||
           tokenAtual(as).tipo == TOKEN_MOD) {
        Token op = consumirToken(as);
        No *no = criarNo(NO_OPERACAO_BIN, op.lexema, op.linha);
        no->tipoToken = op.tipo;
        adicionarFilho(no, esq);
        adicionarFilho(no, analisarFator(as));
        esq = no;
    }

    return esq;
}

/* fator → '!' fator | '-' fator | primario */
static No *analisarFator(AnalisadorSintatico *as) {
    Token atual = tokenAtual(as);

    if (atual.tipo == TOKEN_NAO) {
        consumirToken(as);
        No *no = criarNo(NO_OPERACAO_UN, "!", atual.linha);
        no->tipoToken = TOKEN_NAO;
        adicionarFilho(no, analisarFator(as));
        return no;
    }

    if (atual.tipo == TOKEN_MENOS) {
        consumirToken(as);
        No *no = criarNo(NO_OPERACAO_UN, "-", atual.linha);
        no->tipoToken = TOKEN_MENOS;
        adicionarFilho(no, analisarFator(as));
        return no;
    }

    return analisarPrimario(as);
}

/*
 * Analisa um elemento primário (folha da expressão).
 * Pode ser: número, string, char, bool, variável ou chamada de função.
 */
static No *analisarPrimario(AnalisadorSintatico *as) {
    Token atual = tokenAtual(as);

    switch (atual.tipo) {
        /* Literais numéricos */
        case TOKEN_NUM_INT:
            consumirToken(as);
            return criarNo(NO_NUMERO_INT, atual.lexema, atual.linha);

        case TOKEN_NUM_FLOAT:
            consumirToken(as);
            return criarNo(NO_NUMERO_FLOAT, atual.lexema, atual.linha);

        /* Literais de texto */
        case TOKEN_LITERAL_STR:
            consumirToken(as);
            return criarNo(NO_LITERAL_STR, atual.lexema, atual.linha);

        case TOKEN_LITERAL_CHAR:
            consumirToken(as);
            return criarNo(NO_LITERAL_CHAR, atual.lexema, atual.linha);

        /* Booleanos */
        case TOKEN_TRUE:
            consumirToken(as);
            return criarNo(NO_BOOLEANO, "true", atual.linha);

        case TOKEN_FALSE:
            consumirToken(as);
            return criarNo(NO_BOOLEANO, "false", atual.linha);

        /* Identificador: variável ou chamada de função */
        case TOKEN_IDENTIFICADOR: {
            consumirToken(as);

            /* Chamada de função: nome(args...) */
            if (tokenAtual(as).tipo == TOKEN_ABRE_PAREN) {
                No *chamada = criarNo(NO_CHAMADA_FUNC, atual.lexema, atual.linha);
                consumirToken(as); /* Consome '(' */

                while (tokenAtual(as).tipo != TOKEN_FECHA_PAREN &&
                       tokenAtual(as).tipo != TOKEN_EOF) {
                    No *arg = analisarExpressao(as);
                    adicionarFilho(chamada, arg);
                    if (tokenAtual(as).tipo == TOKEN_VIRGULA) consumirToken(as);
                    else break;
                }
                esperarToken(as, TOKEN_FECHA_PAREN);
                return chamada;
            }

            /* Simples variável */
            return criarNo(NO_IDENTIFICADOR, atual.lexema, atual.linha);
        }

        /* Expressão parentesizada: (expr) */
        case TOKEN_ABRE_PAREN: {
            consumirToken(as); /* Consome '(' */
            No *expr = analisarExpressao(as);
            esperarToken(as, TOKEN_FECHA_PAREN);
            return expr;
        }

        default:
            fprintf(stderr,
                "ERRO SINTATICO [linha %d]: Expressao invalida: '%s'\n",
                atual.linha, atual.lexema);
            as->erros++;
            consumirToken(as);
            return criarNo(NO_NUMERO_INT, "0", atual.linha); /* Nó de recuperação */
    }
}

/*
 * Analisa um tipo de dado.
 *
 * tipo → 'int' | 'float' | 'char' | 'bool' | 'string'
 */
static No *analisarTipo(AnalisadorSintatico *as) {
    Token atual = tokenAtual(as);

    if (ehTipo(atual.tipo)) {
        consumirToken(as);
        No *no = criarNo(NO_TIPO, atual.lexema, atual.linha);
        no->tipoToken = atual.tipo;
        return no;
    }

    fprintf(stderr,
        "ERRO SINTATICO [linha %d]: Tipo invalido: '%s'\n",
        atual.linha, atual.lexema);
    as->erros++;

    /* Retorna 'int' como tipo padrão para recuperação de erro */
    return criarNo(NO_TIPO, "int", atual.linha);
}
