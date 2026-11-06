/*
 * =============================================================================
 * analisadorSemantico.c - Implementação do Analisador Semântico da MACSLang
 * =============================================================================
 *
 * VERIFICAÇÕES REALIZADAS:
 *
 * 1. Variáveis:
 *    - Não podem ser declaradas duas vezes no mesmo escopo
 *    - Não podem ser usadas antes de serem declaradas
 *    - A atribuição deve ter tipo compatível
 *
 * 2. Funções:
 *    - Não podem ser declaradas duas vezes
 *    - Chamadas devem ter número correto de argumentos
 *    - Tipos dos argumentos devem ser compatíveis
 *
 * 3. Tipos:
 *    - Operações só entre tipos compatíveis
 *    - Condições de if/while/for devem ser bool
 *    - Return deve ter tipo compatível com a função
 *
 * =============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "analisadorSemantico.h"

/* Protótipos das funções internas */
static TipoDado analisarNoSemantico(AnalisadorSemantico *sem, No *no);
static TipoDado analisarExpressaoSem(AnalisadorSemantico *sem, No *no);
static void     entrarEscopo(AnalisadorSemantico *sem);
static void     sairEscopo(AnalisadorSemantico *sem);
static Simbolo *inserirSimbolo(AnalisadorSemantico *sem, const char *nome,
                                TipoDado tipo, int ehFuncao, int linha);

/* =============================================================================
 * CONVERSÕES DE TIPO
 * =============================================================================
 */

/*
 * Converte o tipo do token (léxico) para o tipo interno do semântico.
 */
TipoDado tokenParaTipo(TipoToken token) {
    switch (token) {
        case TOKEN_TIPO_INT:    return TIPO_INT;
        case TOKEN_TIPO_FLOAT:  return TIPO_FLOAT;
        case TOKEN_TIPO_CHAR:   return TIPO_CHAR;
        case TOKEN_TIPO_BOOL:   return TIPO_BOOL;
        case TOKEN_TIPO_STRING: return TIPO_STRING;
        default:                return TIPO_ERRO;
    }
}

/*
 * Retorna o nome do tipo como string legível.
 */
const char *nomeTipo(TipoDado tipo) {
    switch (tipo) {
        case TIPO_INT:    return "int";
        case TIPO_FLOAT:  return "float";
        case TIPO_CHAR:   return "char";
        case TIPO_BOOL:   return "bool";
        case TIPO_STRING: return "string";
        case TIPO_VOID:   return "void";
        case TIPO_ERRO:   return "ERRO";
        default:          return "desconhecido";
    }
}

/*
 * Verifica se dois tipos são compatíveis para operações aritméticas.
 * int e float são compatíveis entre si (promoção automática).
 */
static int tiposCompativeis(TipoDado t1, TipoDado t2) {
    if (t1 == t2) return 1;
    if (t1 == TIPO_ERRO || t2 == TIPO_ERRO) return 1; /* Não gera erro duplo */
    /* Int e float são compatíveis */
    if ((t1 == TIPO_INT || t1 == TIPO_FLOAT) &&
        (t2 == TIPO_INT || t2 == TIPO_FLOAT)) return 1;
    return 0;
}

/*
 * Retorna o tipo resultante de uma operação entre dois tipos.
 * Se um deles é float, o resultado é float (promoção).
 */
static TipoDado tipoResultante(TipoDado t1, TipoDado t2) {
    if (t1 == TIPO_ERRO || t2 == TIPO_ERRO) return TIPO_ERRO;
    if (t1 == TIPO_FLOAT || t2 == TIPO_FLOAT) return TIPO_FLOAT;
    if (t1 == TIPO_STRING || t2 == TIPO_STRING) return TIPO_STRING;
    return t1;
}

/* =============================================================================
 * GERENCIAMENTO DA TABELA DE SÍMBOLOS
 * =============================================================================
 */

/*
 * Entra em um novo escopo (ao entrar em um bloco {}).
 */
static void entrarEscopo(AnalisadorSemantico *sem) {
    sem->tabela.escopoAtual++;
}

/*
 * Sai do escopo atual (ao fechar um bloco {}).
 * Remove todos os símbolos que pertenciam a este escopo.
 */
static void sairEscopo(AnalisadorSemantico *sem) {
    int escopo = sem->tabela.escopoAtual;

    /* Remove símbolos do escopo que está sendo fechado */
    int i = sem->tabela.numSimbolos - 1;
    while (i >= 0 && sem->tabela.simbolos[i].escopo == escopo) {
        i--;
    }
    sem->tabela.numSimbolos = i + 1;

    sem->tabela.escopoAtual--;
}

/*
 * Insere um novo símbolo na tabela.
 * Verifica duplicação no mesmo escopo.
 */
static Simbolo *inserirSimbolo(AnalisadorSemantico *sem, const char *nome,
                                TipoDado tipo, int ehFuncao, int linha) {
    TabelaSimbolos *tab = &sem->tabela;

    /* Verifica se já existe no escopo atual */
    for (int i = 0; i < tab->numSimbolos; i++) {
        if (strcmp(tab->simbolos[i].nome, nome) == 0 &&
            tab->simbolos[i].escopo == tab->escopoAtual) {
            fprintf(stderr,
                "ERRO SEMANTICO [linha %d]: '%s' ja foi declarado neste escopo\n",
                linha, nome);
            sem->erros++;
            return &tab->simbolos[i];
        }
    }

    /* Insere novo símbolo */
    if (tab->numSimbolos >= MAX_SIMBOLOS) {
        fprintf(stderr, "ERRO: Tabela de simbolos cheia!\n");
        exit(1);
    }

    Simbolo *s = &tab->simbolos[tab->numSimbolos++];
    strncpy(s->nome, nome, MAX_LEXEMA - 1);
    s->tipo         = tipo;
    s->ehFuncao     = ehFuncao;
    s->escopo       = tab->escopoAtual;
    s->linha        = linha;
    s->numParams    = 0;
    s->enderecoAsm  = 0;

    return s;
}

/*
 * Busca um símbolo em todos os escopos (do mais interno para o mais externo).
 * Retorna NULL se não encontrar.
 */
Simbolo *buscarSimbolo(TabelaSimbolos *tabela, const char *nome) {
    /* Busca de trás para frente para encontrar o escopo mais interno primeiro */
    for (int i = tabela->numSimbolos - 1; i >= 0; i--) {
        if (strcmp(tabela->simbolos[i].nome, nome) == 0) {
            return &tabela->simbolos[i];
        }
    }
    return NULL;
}

/*
 * Imprime a tabela de símbolos para debug.
 */
void imprimirTabelaSimbolos(TabelaSimbolos *tabela) {
    printf("\n=== TABELA DE SIMBOLOS ===\n");
    printf("%-20s %-10s %-8s %-8s\n", "Nome", "Tipo", "Funcao", "Escopo");
    printf("%-20s %-10s %-8s %-8s\n", "----", "----", "------", "------");

    for (int i = 0; i < tabela->numSimbolos; i++) {
        Simbolo *s = &tabela->simbolos[i];
        printf("%-20s %-10s %-8s %-8d\n",
               s->nome,
               nomeTipo(s->tipo),
               s->ehFuncao ? "sim" : "nao",
               s->escopo);
    }
    printf("==========================\n\n");
}

/* =============================================================================
 * ANÁLISE SEMÂNTICA DOS NÓS DA AST
 * =============================================================================
 */

/*
 * Inicializa o analisador semântico.
 */
void iniciarSemantico(AnalisadorSemantico *sem) {
    sem->tabela.numSimbolos = 0;
    sem->tabela.escopoAtual = 0; /* Escopo global */
    sem->erros              = 0;
    sem->funcaoAtual[0]     = '\0';
    sem->tipoRetornoAtual   = TIPO_VOID;
}

/*
 * Ponto de entrada: analisa toda a AST.
 */
void analisarSemantico(AnalisadorSemantico *sem, No *raiz) {
    if (!raiz) return;
    analisarNoSemantico(sem, raiz);

    if (sem->erros > 0) {
        fprintf(stderr, "\nTotal de erros semanticos: %d\n", sem->erros);
    }
}

/*
 * Analisa um nó da AST e retorna seu tipo de dado.
 * Esta função é chamada recursivamente para todos os nós.
 */
static TipoDado analisarNoSemantico(AnalisadorSemantico *sem, No *no) {
    if (!no) return TIPO_VOID;

    switch (no->tipo) {

        /* ----------------------------------------------------------------
         * PROGRAMA: analisa todas as declarações globais
         * ---------------------------------------------------------------- */
        case NO_PROGRAMA:
            for (int i = 0; i < no->numFilhos; i++) {
                analisarNoSemantico(sem, no->filhos[i]);
            }
            return TIPO_VOID;

        /* ----------------------------------------------------------------
         * DECLARAÇÃO DE VARIÁVEL: var x: tipo = valor;
         * ---------------------------------------------------------------- */
        case NO_DECL_VAR: {
            /* filhos[0] = tipo, filhos[1] = valor inicial (opcional) */
            No *noTipo = no->filhos[0];
            TipoDado tipo = tokenParaTipo(noTipo->tipoToken);

            /* Insere na tabela de símbolos */
            inserirSimbolo(sem, no->valor, tipo, 0, no->linha);

            /* Verifica tipo do valor inicial se houver */
            if (no->numFilhos > 1) {
                TipoDado tipoValor = analisarExpressaoSem(sem, no->filhos[1]);
                if (!tiposCompativeis(tipo, tipoValor) && tipoValor != TIPO_ERRO) {
                    fprintf(stderr,
                        "ERRO SEMANTICO [linha %d]: Tipo incompativel: "
                        "variavel '%s' e '%s' mas valor e '%s'\n",
                        no->linha, no->valor, nomeTipo(tipo), nomeTipo(tipoValor));
                    sem->erros++;
                }
            }
            return TIPO_VOID;
        }

        /* ----------------------------------------------------------------
         * DECLARAÇÃO DE FUNÇÃO: func nome(params): tipo { corpo }
         * ---------------------------------------------------------------- */
        case NO_DECL_FUNC: {
            /*
             * Estrutura dos filhos:
             * [0..n-2] = parâmetros (NO_PARAMETRO)
             * [n-2]    = tipo de retorno (NO_TIPO)
             * [n-1]    = corpo (NO_BLOCO)
             */
            int nFilhos = no->numFilhos;
            No *noTipoRet = no->filhos[nFilhos - 2];
            TipoDado tipoRet = tokenParaTipo(noTipoRet->tipoToken);

            /* Insere a função na tabela */
            Simbolo *simFunc = inserirSimbolo(sem, no->valor, tipoRet, 1, no->linha);

            /* Conta e registra os parâmetros */
            int numParams = nFilhos - 2; /* Exclui tipo retorno e bloco */
            simFunc->numParams = numParams;

            /* Entra no escopo da função */
            entrarEscopo(sem);

            /* Registra parâmetros no escopo da função */
            for (int i = 0; i < numParams; i++) {
                No *param = no->filhos[i];
                No *paramTipo = param->filhos[0];
                TipoDado tp = tokenParaTipo(paramTipo->tipoToken);
                simFunc->tiposParams[i] = tp;
                inserirSimbolo(sem, param->valor, tp, 0, param->linha);
            }

            /* Salva contexto da função para verificar return */
            char funcAnterior[MAX_LEXEMA];
            TipoDado retAnterior = sem->tipoRetornoAtual;
            strncpy(funcAnterior, sem->funcaoAtual, MAX_LEXEMA - 1);
            strncpy(sem->funcaoAtual, no->valor, MAX_LEXEMA - 1);
            sem->tipoRetornoAtual = tipoRet;

            /* Analisa o corpo da função */
            No *corpo = no->filhos[nFilhos - 1];
            analisarNoSemantico(sem, corpo);

            /* Restaura contexto anterior */
            strncpy(sem->funcaoAtual, funcAnterior, MAX_LEXEMA - 1);
            sem->tipoRetornoAtual = retAnterior;

            sairEscopo(sem);
            return TIPO_VOID;
        }

        /* ----------------------------------------------------------------
         * BLOCO: { declarações... }
         * ---------------------------------------------------------------- */
        case NO_BLOCO:
            entrarEscopo(sem);
            for (int i = 0; i < no->numFilhos; i++) {
                analisarNoSemantico(sem, no->filhos[i]);
            }
            sairEscopo(sem);
            return TIPO_VOID;

        /* ----------------------------------------------------------------
         * ATRIBUIÇÃO: x = expressão;
         * ---------------------------------------------------------------- */
        case NO_ATRIBUICAO: {
            Simbolo *s = buscarSimbolo(&sem->tabela, no->valor);
            if (!s) {
                fprintf(stderr,
                    "ERRO SEMANTICO [linha %d]: Variavel '%s' nao declarada\n",
                    no->linha, no->valor);
                sem->erros++;
                return TIPO_ERRO;
            }

            TipoDado tipoExpr = analisarExpressaoSem(sem, no->filhos[0]);
            if (!tiposCompativeis(s->tipo, tipoExpr) && tipoExpr != TIPO_ERRO) {
                fprintf(stderr,
                    "ERRO SEMANTICO [linha %d]: Atribuicao incompativel: "
                    "'%s' e '%s' mas expressao e '%s'\n",
                    no->linha, no->valor, nomeTipo(s->tipo), nomeTipo(tipoExpr));
                sem->erros++;
            }
            return TIPO_VOID;
        }

        /* ----------------------------------------------------------------
         * IF: if (cond) { } else { }
         * ---------------------------------------------------------------- */
        case NO_IF: {
            TipoDado tipoCond = analisarExpressaoSem(sem, no->filhos[0]);
            /* Condição deve ser bool (ou int, aceito por compatibilidade) */
            if (tipoCond != TIPO_BOOL && tipoCond != TIPO_INT && tipoCond != TIPO_ERRO) {
                fprintf(stderr,
                    "ERRO SEMANTICO [linha %d]: Condicao do 'if' deve ser bool\n",
                    no->linha);
                sem->erros++;
            }
            /* Analisa os blocos */
            for (int i = 1; i < no->numFilhos; i++) {
                analisarNoSemantico(sem, no->filhos[i]);
            }
            return TIPO_VOID;
        }

        /* ----------------------------------------------------------------
         * WHILE: while (cond) { }
         * ---------------------------------------------------------------- */
        case NO_WHILE: {
            TipoDado tipoCond = analisarExpressaoSem(sem, no->filhos[0]);
            if (tipoCond != TIPO_BOOL && tipoCond != TIPO_INT && tipoCond != TIPO_ERRO) {
                fprintf(stderr,
                    "ERRO SEMANTICO [linha %d]: Condicao do 'while' deve ser bool\n",
                    no->linha);
                sem->erros++;
            }
            analisarNoSemantico(sem, no->filhos[1]);
            return TIPO_VOID;
        }

        /* ----------------------------------------------------------------
         * FOR: for (init; cond; incr) { }
         * ---------------------------------------------------------------- */
        case NO_FOR:
            /* filhos: [0]=init, [1]=condição, [2]=incremento, [3]=corpo */
            /* init pode ser decl_var ou atribuição */
            entrarEscopo(sem);
            analisarNoSemantico(sem, no->filhos[0]); /* init */
            analisarExpressaoSem(sem, no->filhos[1]); /* condição */
            analisarNoSemantico(sem, no->filhos[2]); /* incremento */
            /* corpo é o NO_BLOCO - analisa sem entrar em novo escopo extra */
            if (no->numFilhos > 3) {
                /* Analisa diretamente os filhos do bloco (já estamos no escopo) */
                No *corpo = no->filhos[3];
                for (int i = 0; i < corpo->numFilhos; i++) {
                    analisarNoSemantico(sem, corpo->filhos[i]);
                }
            }
            sairEscopo(sem);
            return TIPO_VOID;

        /* ----------------------------------------------------------------
         * RETURN: return expressão;
         * ---------------------------------------------------------------- */
        case NO_RETURN: {
            TipoDado tipoRet = TIPO_VOID;
            if (no->numFilhos > 0) {
                tipoRet = analisarExpressaoSem(sem, no->filhos[0]);
            }
            if (!tiposCompativeis(sem->tipoRetornoAtual, tipoRet) &&
                tipoRet != TIPO_ERRO) {
                fprintf(stderr,
                    "ERRO SEMANTICO [linha %d]: Tipo de retorno incompativel em '%s': "
                    "esperado '%s', encontrado '%s'\n",
                    no->linha, sem->funcaoAtual,
                    nomeTipo(sem->tipoRetornoAtual), nomeTipo(tipoRet));
                sem->erros++;
            }
            return TIPO_VOID;
        }

        /* ----------------------------------------------------------------
         * PRINT: print(expressão);
         * ---------------------------------------------------------------- */
        case NO_PRINT:
            if (no->numFilhos > 0) {
                analisarExpressaoSem(sem, no->filhos[0]);
            }
            return TIPO_VOID;

        /* ----------------------------------------------------------------
         * INPUT: input(variável);
         * ---------------------------------------------------------------- */
        case NO_INPUT: {
            No *varNo = no->filhos[0];
            Simbolo *s = buscarSimbolo(&sem->tabela, varNo->valor);
            if (!s) {
                fprintf(stderr,
                    "ERRO SEMANTICO [linha %d]: Variavel '%s' nao declarada\n",
                    no->linha, varNo->valor);
                sem->erros++;
            }
            return TIPO_VOID;
        }

        /* Para expressões, delega para analisarExpressaoSem */
        default:
            return analisarExpressaoSem(sem, no);
    }
}

/*
 * Analisa uma expressão e retorna seu tipo.
 * Verifica compatibilidade de tipos em operações.
 */
static TipoDado analisarExpressaoSem(AnalisadorSemantico *sem, No *no) {
    if (!no) return TIPO_VOID;

    switch (no->tipo) {

        /* Literais têm tipos fixos */
        case NO_NUMERO_INT:   return TIPO_INT;
        case NO_NUMERO_FLOAT: return TIPO_FLOAT;
        case NO_LITERAL_CHAR: return TIPO_CHAR;
        case NO_LITERAL_STR:  return TIPO_STRING;
        case NO_BOOLEANO:     return TIPO_BOOL;

        /* Identificador: busca na tabela */
        case NO_IDENTIFICADOR: {
            Simbolo *s = buscarSimbolo(&sem->tabela, no->valor);
            if (!s) {
                fprintf(stderr,
                    "ERRO SEMANTICO [linha %d]: Variavel '%s' nao declarada\n",
                    no->linha, no->valor);
                sem->erros++;
                return TIPO_ERRO;
            }
            return s->tipo;
        }

        /* Chamada de função: verifica número e tipos de argumentos */
        case NO_CHAMADA_FUNC: {
            Simbolo *func = buscarSimbolo(&sem->tabela, no->valor);
            if (!func) {
                fprintf(stderr,
                    "ERRO SEMANTICO [linha %d]: Funcao '%s' nao declarada\n",
                    no->linha, no->valor);
                sem->erros++;
                return TIPO_ERRO;
            }
            if (!func->ehFuncao) {
                fprintf(stderr,
                    "ERRO SEMANTICO [linha %d]: '%s' e uma variavel, nao funcao\n",
                    no->linha, no->valor);
                sem->erros++;
                return TIPO_ERRO;
            }
            /* Verifica número de argumentos */
            if (no->numFilhos != func->numParams) {
                fprintf(stderr,
                    "ERRO SEMANTICO [linha %d]: Funcao '%s' espera %d args, "
                    "recebeu %d\n",
                    no->linha, no->valor, func->numParams, no->numFilhos);
                sem->erros++;
            }
            /* Verifica tipos dos argumentos */
            int nVerif = no->numFilhos < func->numParams
                       ? no->numFilhos : func->numParams;
            for (int i = 0; i < nVerif; i++) {
                TipoDado tipoArg = analisarExpressaoSem(sem, no->filhos[i]);
                if (!tiposCompativeis(func->tiposParams[i], tipoArg) &&
                    tipoArg != TIPO_ERRO) {
                    fprintf(stderr,
                        "ERRO SEMANTICO [linha %d]: Arg %d de '%s': "
                        "esperado '%s', recebeu '%s'\n",
                        no->linha, i + 1, no->valor,
                        nomeTipo(func->tiposParams[i]), nomeTipo(tipoArg));
                    sem->erros++;
                }
            }
            return func->tipo;
        }

        /* Operação binária: verifica tipos dos operandos */
        case NO_OPERACAO_BIN: {
            TipoDado t1 = analisarExpressaoSem(sem, no->filhos[0]);
            TipoDado t2 = analisarExpressaoSem(sem, no->filhos[1]);

            /* Operadores de comparação sempre retornam bool */
            TipoToken op = no->tipoToken;
            if (op == TOKEN_IGUAL_IGUAL || op == TOKEN_DIFERENTE ||
                op == TOKEN_MENOR       || op == TOKEN_MAIOR      ||
                op == TOKEN_MENOR_IGUAL || op == TOKEN_MAIOR_IGUAL) {
                if (!tiposCompativeis(t1, t2)) {
                    fprintf(stderr,
                        "ERRO SEMANTICO [linha %d]: Comparacao entre tipos "
                        "incompativeis: '%s' e '%s'\n",
                        no->linha, nomeTipo(t1), nomeTipo(t2));
                    sem->erros++;
                }
                return TIPO_BOOL;
            }

            /* Operadores lógicos sempre retornam bool */
            if (op == TOKEN_E || op == TOKEN_OU) {
                return TIPO_BOOL;
            }

            /* Concatenação de string com + */
            if (op == TOKEN_MAIS &&
                (t1 == TIPO_STRING || t2 == TIPO_STRING)) {
                return TIPO_STRING;
            }

            /* Operações aritméticas */
            if (!tiposCompativeis(t1, t2)) {
                fprintf(stderr,
                    "ERRO SEMANTICO [linha %d]: Operacao '%s' entre tipos "
                    "incompativeis: '%s' e '%s'\n",
                    no->linha, no->valor, nomeTipo(t1), nomeTipo(t2));
                sem->erros++;
                return TIPO_ERRO;
            }

            return tipoResultante(t1, t2);
        }

        /* Operação unária */
        case NO_OPERACAO_UN: {
            TipoDado t = analisarExpressaoSem(sem, no->filhos[0]);
            if (no->tipoToken == TOKEN_NAO) return TIPO_BOOL;
            return t;
        }

        default:
            return analisarNoSemantico(sem, no);
    }
}
