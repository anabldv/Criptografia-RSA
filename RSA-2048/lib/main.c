#include "FunRSA.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main ()
{
    uint32_t p = 61; // número primo
    uint32_t q = 53; // número primo

    int opcao = 1; 

    int resultado = rsa(opcao, &p, &q);
    if (resultado == 0) {
        printf("Operacao concluida com sucesso.\n");
    } else {
        printf("Erro na operacao.\n");
    }

    return 0;
}