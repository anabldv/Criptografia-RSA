#include "FunRSA.h"


int main () { srand((unsigned)time(NULL));
    int opcao = -1, debug = 1; 

    s_RSA cifra = {.p = 61, .q = 53};
    char msg[] = "Hello, RSA!"; // Mensagem a ser criptografada
    size_t len = strlen(msg);

    char *decifrada = calloc(len + 1, 1); // Buffer para armazenar a saída
    
    do {
        printf("\nDigite 1 para criptografar | 2 para descriptografar | 3 ver mensagem | 4 para ver as chaves | 0 sair\n--> ");

        if (scanf("%d", &opcao) != 1) {
            int caracter; while ((caracter = getchar()) != '\n' && caracter != EOF);
            opcao = -1;
            printf("\n[ALERTA] Entrada invalida\n");
            continue;
        }

//========================================================================================= Criptografar ou Descriptografar
        if (opcao == 1 || opcao == 2){                                                  
            int resultado = rsa(opcao, &cifra,  msg,  decifrada, debug);
            
            // ------------------------------- Sucesso
            if (resultado == 0) {
                printf("\n\nOperacao concluida com sucesso.\n\n");

            // ------------------------------- Erro
            } else {
                printf("\n\nErro na operacao. Codigo de erro: %d\n", resultado);
                if (debug) printf("\n********* Codigos de Erro *********\n");
                if (debug) printf("\n\t1) verifica_p_q\n\t2) calcula_bases\n\t3) chave_publica\n\t4) chave_privada\n\t5) criptografar\n\t6) descriptografar\n");
            }

//=========================================================================================  Ver chaves
        } else if (opcao ==  4) {  
            if (cifra.pronto){
                printf("\nChave publica: e: %u, n: %u",   cifra.e, cifra.n);
                printf("\nChave privada: d: %u, n: %u\n", cifra.d, cifra.n);                
            } 
            else printf("\n[ALERTA] Gere as chaves primeiro\n");
              
//========================================================================================= Ver Mensagem criptografada
        } else if (opcao ==  3) {   
            printf("\nMensagem original: %s\nMensagem criptografada: ", msg);

            if (cifra.pronto){                                                      
                for (size_t i = 0; i < cifra.tam; i++) 
                    printf("%d ", cifra.cifra[i]);

                if (debug) printf("\n[DEBUG] cifra.tam: %d\n", cifra.tam);
            } else printf("[ALERTA] Criptografia ainda nao gerada");     
            
            printf("\n");
             
//========================================================================================= Liga/Desliga Debug
        } else if (opcao == 99) debug = !debug;

//========================================================================================= Encerra o Programa
        else if (opcao == 0) printf("\nPrograma encerrado...\n\n");

//========================================================================================= ERRO
        else printf("\n[ALERTA] Erro digite uma opcao valida\n");

    } while (opcao != 0) ;

    free(decifrada);   // Libera a memória alocada para decifrada
    free(cifra.cifra); // Libera a memória alocada para cifra

    return 0;
}
