#include "FunRSA.h"


// Le uma linha de qualquer tamanho (o texto e sempre digitado em tempo de execucao)
static char *le_linha(const char *aviso) {
    size_t cap = 256, len = 0;
    char *buf = malloc(cap);
    int caracter;

    if (!buf) return NULL;
    printf("%s", aviso);
    fflush(stdout);

    while ((caracter = getchar()) != '\n' && caracter != EOF) {
        if (len + 1 >= cap) {
            char *nova = realloc(buf, cap *= 2);
            if (!nova) { free(buf); return NULL; }
            buf = nova;
        }
        buf[len++] = (char)caracter;
    }
    if (caracter == EOF && len == 0) { free(buf); return NULL; } // fim da entrada

    if (len > 0 && buf[len - 1] == '\r') len--;
    buf[len] = '\0';
    return buf;
}

// Pergunta o formato. Enter = NULL, que a biblioteca trata como decimal (padrao)
static char *pergunta_formato(void) {
    char *formato = le_linha("Formato [decimal | hex | base64] (Enter = decimal): ");
    if (formato && formato[0] == '\0') { free(formato); formato = NULL; }
    return formato;
}


int main () {
    int opcao = -1, debug = 1;

    s_RSA cifra = {0};       // bits = 0 -> RSA_BITS_PADRAO (2048)
    char *msg = NULL;        // mensagem original digitada pelo usuario
    char *entrada = NULL;
    char *saida = NULL;

    do {
        printf("\nDigite 1 para criptografar | 2 para descriptografar | 3 ver mensagem | 4 para ver as chaves | 5 gerar chaves | 0 sair\n--> ");
        fflush(stdout);

        free(entrada);
        entrada = le_linha("");
        if (entrada == NULL) { opcao = 0; break; } // fim da entrada

        char *fim;
        opcao = (int)strtol(entrada, &fim, 10);
        if (fim == entrada || *fim != '\0') {
            opcao = -1;
            printf("\n[ALERTA] Entrada invalida\n");
            continue;
        }

//========================================================================================= Criptografar
        if (opcao == 1){
            free(msg);
            msg = le_linha("Digite a mensagem a ser criptografada: ");
            char *formato = pergunta_formato();

            int resultado = rsa(1, &cifra, msg, formato, &saida, debug);

            // ------------------------------- Sucesso
            if (resultado == 0) {
                printf("\n\nOperacao concluida com sucesso.\n\nMensagem criptografada (%llu bloco(s)):\n%s\n",
                       (unsigned long long)cifra.tam, saida);
                rsa_liberar(saida); saida = NULL;

            // ------------------------------- Erro
            } else {
                printf("\n\nErro na operacao. Codigo de erro: %d\n", resultado);
                if (debug) printf("\n********* Codigos de Erro *********\n");
                if (debug) printf("\n\t1) verifica_p_q\n\t2) calcula_bases\n\t3) chave_publica\n\t4) chave_privada\n\t5) criptografar\n\t6) descriptografar\n\t7) extrai_chaves\n");
            }
            free(formato);

//========================================================================================= Descriptografar
        } else if (opcao == 2){
            char *texto = le_linha("Cole o texto criptografado (Enter = usar a ultima cifra): ");
            const char *alvo = (texto && texto[0]) ? texto : cifra.cifra;
            char *formato = pergunta_formato();

            int resultado = rsa(2, &cifra, alvo, formato, &saida, debug);

            if (resultado == 0) {
                printf("\n\nOperacao concluida com sucesso.\n\nMensagem descriptografada: %s\n", saida);
                rsa_liberar(saida); saida = NULL;
            } else {
                printf("\n\nErro na operacao. Codigo de erro: %d\n", resultado);
                if (debug) printf("\n********* Codigos de Erro *********\n");
                if (debug) printf("\n\t1) verifica_p_q\n\t2) calcula_bases\n\t3) chave_publica\n\t4) chave_privada\n\t5) criptografar\n\t6) descriptografar\n\t7) extrai_chaves\n");
            }
            free(texto);
            free(formato);

//=========================================================================================  Ver chaves
        } else if (opcao ==  4) {
            if (cifra.chaves){
                char *formato = pergunta_formato();
                char *e = NULL, *d = NULL, *n1 = NULL, *n2 = NULL;

                if (rsa_chave_publica(&cifra, formato, &e, &n1, debug) == 0)
                    printf("\nChave publica: e: %s\n               n: %s\n", e, n1);
                if (rsa_chave_privada(&cifra, formato, &d, &n2, debug) == 0)
                    printf("\nChave privada: d: %s\n               n: %s\n", d, n2);

                rsa_liberar(e); rsa_liberar(n1); rsa_liberar(d); rsa_liberar(n2);
                free(formato);
            }
            else printf("\n[ALERTA] Gere as chaves primeiro\n");

//========================================================================================= Ver Mensagem criptografada
        } else if (opcao ==  3) {
            printf("\nMensagem original: %s\nMensagem criptografada: ", msg ? msg : "(nenhuma)");

            if (cifra.pronto){
                printf("%s", cifra.cifra);
                if (debug) printf("\n[DEBUG] cifra.tam: %llu\n", (unsigned long long)cifra.tam);
            } else printf("[ALERTA] Criptografia ainda nao gerada");

            printf("\n");

//========================================================================================= Gerar chaves (p e q informados ou aleatorios)
        } else if (opcao ==  5) {
            char *bits = le_linha("Tamanho de n em bits (Enter = 2048): ");
            char *resp = le_linha("Informar p e q manualmente? (s = sim | Enter = gerar aleatorios): ");
            int resultado = 0;

            cifra.bits = (bits && bits[0]) ? atoi(bits) : 0;

            if (resp && (resp[0] == 's' || resp[0] == 'S')) {
                char *p = le_linha("p: ");
                char *q = le_linha("q: ");
                char *formato = pergunta_formato();
                resultado = rsa_define_primos(&cifra, p, q, formato, debug);
                free(p); free(q); free(formato);
            } else rsa_define_primos(&cifra, NULL, NULL, NULL, debug);

            if (resultado == 0) resultado = rsa(3, &cifra, NULL, NULL, NULL, debug);

            if (resultado == 0) printf("\n\nOperacao concluida com sucesso. Chaves geradas (n com %d bits).\n", cifra.bits ? cifra.bits : RSA_BITS_PADRAO);
            else {
                printf("\n\nErro na operacao. Codigo de erro: %d\n", resultado);
                if (debug) printf("\n********* Codigos de Erro *********\n");
                if (debug) printf("\n\t1) verifica_p_q\n\t2) calcula_bases\n\t3) chave_publica\n\t4) chave_privada\n\t5) criptografar\n\t6) descriptografar\n\t7) extrai_chaves\n");
            }
            free(bits); free(resp);

//========================================================================================= Liga/Desliga Debug
        } else if (opcao == 99) debug = !debug;

//========================================================================================= Encerra o Programa
        else if (opcao == 0) printf("\nPrograma encerrado...\n\n");

//========================================================================================= ERRO
        else printf("\n[ALERTA] Erro digite uma opcao valida\n");

    } while (opcao != 0) ;

    free(entrada);
    free(msg);        // Libera a memória alocada para a mensagem
    rsa_limpa(&cifra); // Libera a memória alocada para cifra e apaga as chaves

    return 0;
}
