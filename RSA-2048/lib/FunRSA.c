#include "FunRSA.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define QTD_BLOCO  130


static int verifica_primo(uint32_t *num) {
    if (*num <= 1) return -1; 
    
    for (uint32_t i = 2; i * i <= *num; i++) {
        if (*num % i == 0) return -1; 
    }

    return 0; 
}

static int verifica_p_q(uint32_t *p, uint32_t *q) {

    if (verifica_primo(p) != 0) return -1; // p nao é primo
    if (verifica_primo(q) != 0)  return -1; // q nao é primo
    if (*p <= 1 || *q <= 1) return -1; // p e q devem ser maiores que 1.\n"); 

    return 0; 
}

static int mdc(uint32_t a, uint32_t b, uint32_t *resultado) {
    while (b != 0) {
        uint32_t temp = b;
        b = a % b;
        a = temp;
    }
    *resultado = a; 
    return 0; 
}

 static int chave_publica(uint32_t *p, uint32_t *q, uint32_t *n, uint32_t *e) {
    if (verifica_p_q(p, q) != 0) return -1; // p ou q nao sao primos

    *n = (*p) * (*q); // Calcula n = p * q

    uint32_t phi = (*p - 1) * (*q - 1); // Calcula totiente

    mdc(phi, 65537, e); // Calcula o máximo divisor comum (MDC) entre φ(n) e 65537
    if (*e != 1) return -1; // Verifica se o MDC é igual a 1, caso contrário, não é possível calcular a chave pública.
    *e = 65537; // Valor padrão para e

    return 0; // Sucesso
}

int rsa(int opcao, uint32_t *p, uint32_t *q, char *msg) {
    
    if (verifica_p_q(p, q) != 0) return -1;
    
    if (opcao == 1) {


    } else if (opcao == 2) {


    } else {

        return -1; 
    }
    return 0; // Sucesso

}