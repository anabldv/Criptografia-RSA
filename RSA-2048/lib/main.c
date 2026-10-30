#include "FunRSA.h"


int main ()
{
    s_RSA cifra = {.p = 61, .q = 53};

    int opcao = 0, debug = 1; 

    char msg[] = "Hello, RSA!"; // Mensagem a ser criptografada
    size_t len = strlen(msg);
    int *output = malloc(len * sizeof(int)); // Buffer para armazenar a saída

    do {
        printf("\nDigite 1 para criptografar | 2 para descriptografar | 3 ver mensagem | 4 para ver as chaves | 0 sair\n--> ");
        scanf("%d", &opcao);

//=========================================================================== Criptografar ou Descriptografar
        if (opcao == 1 || opcao == 2){                                                  
            int resultado = rsa(opcao, &cifra,  msg,  output, debug);
            
            // ------------------------------- Sucesso
            if (resultado == 0) {
                printf("\nMensagem criptografada: %s\n", output);
                printf("\nOperacao concluida com sucesso.\n");

            // ------------------------------- Erro
            } else {
                printf("\n\nErro na operacao. Codigo de erro: %d\n", resultado);
                printf("\n********* Codigos de Erro *********\n");
                printf("\n\t1) verifica_p_q\n\t2) calcula_bases\n\t3) chave_publica\n\t4) chave_privada\n\t5) criptografar\n");
            }

//===========================================================================  Ver chaves
        } else if (opcao ==  4) {                                                       
            printf("\nChave publica: e:%u, n:%u\n", cifra.e, cifra.n);
            printf("Chave privada: d:%u, n:%u\n", cifra.d, cifra.n);

//=========================================================================== Ver Mensagem criptografada
        } else if (opcao ==  3) {                                                        
            printf("\nMensagem antes da criptografia: %s\n", msg);
            printf("Mensagem criptografada: ");
            if (debug == 1) printf("\n%d\n",len);
            for (int i = 0; i < len; i++) printf("%d ", output[i]);
            printf("\n");

//=========================================================================== Liga/Desliga Debug
        } else if (opcao == 99){
            debug = !debug;

//=========================================================================== Encerra o Programa
        } else if (opcao == 0){
            printf("\nPrograma encerrado\n\n");

//=========================================================================== ERRO
        } else { printf("\nErro digite uma opcao valida\n"); }

    } while (opcao != 0) ;

    free(output); // Libera a memória alocada para output
    return 0;
}
