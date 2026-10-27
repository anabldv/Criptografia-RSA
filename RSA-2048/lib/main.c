#include "FunRSA.h"


int main ()
{
    uint32_t p = 61; // número primo
    uint32_t q = 53; // número primo

    int opcao = 0; 

    uint32_t n, e, d; // Variáveis para armazenar n, e e d

    char msg[] = "Hello, RSA!"; // Mensagem a ser criptografada
    char *output = malloc((strlen(msg) * 8) + 1 * sizeof(char)); // Buffer para armazenar a saída

    do {
        printf("\nDigite 1 para criptografar, 2 para descriptografar, 3 ver mensagem, 4 para ver as chaves ou 0 para sair: ");
        scanf("%d", &opcao);

        if (opcao == 1 || opcao == 2){ // Criptografar ou Descriptografar
            int resultado = rsa(opcao, &p, &q,  msg, &n, &e, &d, output);

            if (resultado == 0) {
                printf("\nMensagem criptografada: %s\n", output);
                printf("Operacao concluida com sucesso.\n");
            } else {
                printf("\nErro na operacao. Codigo de erro: %d\n", resultado);
            }
        
        } else if (opcao ==  4) { // Ver chaves
            printf("\nChave publica: e:%u, n:%u\n", e, n);
            printf("Chave privada: d:%u, n:%u\n", d, n);
        
        } else if (opcao ==  3) { // Ver Mensagem criptografada
            printf("\nMensagem antes da criptografia: %s\n", msg);
            printf("Mensagem criptografada: %s\n", output);
        
        } else {
            printf("\nErro digite uma opcao valida\n");
        }

    } while (opcao != 0) ;

    free(output); // Libera a memória alocada para output

    return 0;
}
