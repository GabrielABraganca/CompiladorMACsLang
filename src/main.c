/*
 * =============================================================================
 * main.c - Ponto de Entrada do Compilador MACSLang
 * =============================================================================
 *
 * Este arquivo coordena todas as fases do compilador:
 *
 *   FASE 1: Analisador Léxico    → Código fonte → Tokens
 *   FASE 2: Analisador Sintático → Tokens → AST
 *   FASE 3: Analisador Semântico → AST → AST verificada + Tabela de Símbolos
 *   FASE 4: Gerador de Código    → AST → Código Assembly
 *
 * USO:
 *   ./compilador <arquivo.mac> [opcoes]
 *
 * OPÇÕES:
 *   --tokens     Exibe os tokens gerados pelo léxico
 *   --ast        Exibe a AST gerada pelo sintático
 *   --simbolos   Exibe a tabela de símbolos
 *   --verbose    Habilita todas as opções de debug
 *
 * EXEMPLOS:
 *   ./compilador testes/teste1.mac
 *   ./compilador testes/teste2.mac --ast --tokens
 *
 * =============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "analisadorLexico.h"
#include "analisadorSintatico.h"
#include "analisadorSemantico.h"
#include "geradorCodigo.h"

/* Nome padrão do arquivo de saída */
#define ARQUIVO_SAIDA "output/programa.asm"

/*
 * Lê o conteúdo de um arquivo e retorna como string.
 * O chamador é responsável por liberar a memória com free().
 *
 * Parâmetros:
 *   nomeArquivo - caminho do arquivo a ser lido
 *   tamanho     - ponteiro onde o tamanho do arquivo será armazenado
 *
 * Retorna:
 *   Ponteiro para o conteúdo do arquivo, ou NULL em caso de erro.
 */
static char *lerArquivo(const char *nomeArquivo, int *tamanho) {
    FILE *f = fopen(nomeArquivo, "r");
    if (!f) {
        fprintf(stderr, "ERRO: Nao foi possivel abrir '%s'\n", nomeArquivo);
        return NULL;
    }

    /* Descobre o tamanho do arquivo */
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    /* Aloca buffer e lê o arquivo */
    char *buffer = (char *)malloc(sz + 1);
    if (!buffer) {
        fprintf(stderr, "ERRO: Falha ao alocar memoria\n");
        fclose(f);
        return NULL;
    }

    *tamanho = (int)fread(buffer, 1, sz, f);
    buffer[*tamanho] = '\0';
    fclose(f);

    return buffer;
}

/*
 * Exibe o banner do compilador.
 */
static void exibirBanner(void) {
    printf("========================================\n");
    printf("  Compilador MACSLang v1.0\n");
    printf("  Disciplina: Teoria da Computacao e Compiladores\n");
    printf("  Linguagem alvo: x86 Assembly (Windows)\n");
    printf("========================================\n\n");
}

/*
 * Exibe a ajuda de uso.
 */
static void exibirAjuda(const char *prog) {
    printf("Uso: %s <arquivo.mac> [opcoes]\n\n", prog);
    printf("Opcoes:\n");
    printf("  --tokens    Exibe os tokens (Fase 1 - Lexico)\n");
    printf("  --ast       Exibe a arvore sintatica (Fase 2 - Sintatico)\n");
    printf("  --simbolos  Exibe a tabela de simbolos (Fase 3 - Semantico)\n");
    printf("  --verbose   Habilita todas as opcoes de debug\n");
    printf("  --help      Exibe esta mensagem\n");
    printf("\nExemplo:\n");
    printf("  %s testes/teste1.mac --ast\n", prog);
}

/*
 * =============================================================================
 * FUNÇÃO PRINCIPAL
 * =============================================================================
 */
int main(int argc, char *argv[]) {
    exibirBanner();

    /* Verifica argumentos */
    if (argc < 2) {
        exibirAjuda(argv[0]);
        return 1;
    }

    /* Parse dos argumentos de linha de comando */
    const char *arquivoEntrada = argv[1];
    int mostrarTokens  = 0;
    int mostrarAST     = 0;
    int mostrarSimbolo = 0;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--tokens")   == 0) mostrarTokens  = 1;
        if (strcmp(argv[i], "--ast")      == 0) mostrarAST     = 1;
        if (strcmp(argv[i], "--simbolos") == 0) mostrarSimbolo = 1;
        if (strcmp(argv[i], "--verbose")  == 0) {
            mostrarTokens = mostrarAST = mostrarSimbolo = 1;
        }
        if (strcmp(argv[i], "--help") == 0) {
            exibirAjuda(argv[0]);
            return 0;
        }
    }

    printf("[*] Compilando: %s\n", arquivoEntrada);
    printf("[*] Saida:      %s\n\n", ARQUIVO_SAIDA);

    /* =========================================================================
     * LEITURA DO ARQUIVO FONTE
     * ========================================================================= */
    int tamanho;
    char *codigoFonte = lerArquivo(arquivoEntrada, &tamanho);
    if (!codigoFonte) return 1;

    printf("[1/4] Analise Lexica...\n");

    /* =========================================================================
     * FASE 1: ANALISADOR LÉXICO
     *
     * Transforma o código fonte em uma sequência de tokens.
     * ========================================================================= */
    AnalisadorLexico lexico;
    iniciarLexico(&lexico, codigoFonte, tamanho);

    if (mostrarTokens) {
        printf("\n--- TOKENS ---\n");
        /* Para exibir tokens, precisamos de uma cópia do léxico */
        AnalisadorLexico lexicoCopia = lexico;
        Token t;
        int numTok = 0;
        do {
            t = proximoToken(&lexicoCopia);
            if (t.tipo != TOKEN_EOF) {
                printf("  [%03d] %-20s \"%s\" (linha %d)\n",
                       numTok++, nomeToken(t.tipo), t.lexema, t.linha);
            }
        } while (t.tipo != TOKEN_EOF);
        printf("  [FIM] EOF\n");
        printf("--- FIM TOKENS (%d tokens) ---\n\n", numTok);
    }

    printf("[2/4] Analise Sintatica...\n");

    /* =========================================================================
     * FASE 2: ANALISADOR SINTÁTICO
     *
     * Transforma os tokens em uma Árvore Sintática Abstrata (AST).
     * ========================================================================= */
    AnalisadorSintatico sintatico;
    iniciarSintatico(&sintatico, &lexico);
    No *ast = analisarPrograma(&sintatico);

    if (sintatico.erros > 0) {
        fprintf(stderr, "\n[ERRO] %d erro(s) sintatico(s) encontrado(s). "
                "Compilacao interrompida.\n", sintatico.erros);
        liberarAST(ast);
        free(codigoFonte);
        return 1;
    }

    if (mostrarAST) {
        printf("\n--- ARVORE SINTATICA (AST) ---\n");
        imprimirAST(ast, 0);
        printf("--- FIM AST ---\n\n");
    }

    printf("[3/4] Analise Semantica...\n");

    /* =========================================================================
     * FASE 3: ANALISADOR SEMÂNTICO
     *
     * Verifica a consistência do programa (tipos, declarações, etc.)
     * e preenche a tabela de símbolos.
     * ========================================================================= */
    AnalisadorSemantico semantico;
    iniciarSemantico(&semantico);
    analisarSemantico(&semantico, ast);

    if (semantico.erros > 0) {
        fprintf(stderr, "\n[ERRO] %d erro(s) semantico(s) encontrado(s). "
                "Compilacao interrompida.\n", semantico.erros);
        liberarAST(ast);
        free(codigoFonte);
        return 1;
    }

    if (mostrarSimbolo) {
        imprimirTabelaSimbolos(&semantico.tabela);
    }

    printf("[4/4] Geracao de Codigo...\n");

    /* =========================================================================
     * FASE 4: GERADOR DE CÓDIGO
     *
     * Gera o código Assembly x86 para Windows.
     * ========================================================================= */
    FILE *arquivoSaida = fopen(ARQUIVO_SAIDA, "w");
    if (!arquivoSaida) {
        fprintf(stderr, "[ERRO] Nao foi possivel criar '%s'\n", ARQUIVO_SAIDA);
        /* Tenta criar na pasta atual */
        arquivoSaida = fopen("programa.asm", "w");
        if (!arquivoSaida) {
            fprintf(stderr, "[ERRO] Nao foi possivel criar 'programa.asm'\n");
            liberarAST(ast);
            free(codigoFonte);
            return 1;
        }
        printf("       (Saida alternativa: programa.asm)\n");
    }

    GeradorCodigo gerador;
    iniciarGerador(&gerador, arquivoSaida, &semantico.tabela);
    gerarCodigo(&gerador, ast);
    fclose(arquivoSaida);

    /* =========================================================================
     * LIMPEZA E RESULTADO
     * ========================================================================= */
    liberarAST(ast);
    free(codigoFonte);

    printf("\n========================================\n");
    printf("  Compilacao concluida com SUCESSO!\n");
    printf("  Arquivo gerado: %s\n", ARQUIVO_SAIDA);
    printf("\n  Para montar e linkar (Windows):\n");
    printf("    nasm -f win32 output/programa.asm -o programa.obj\n");
    printf("    gcc -m32 programa.obj -o programa.exe\n");
    printf("    programa.exe\n");
    printf("========================================\n");

    return 0;
}
