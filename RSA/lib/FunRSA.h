#ifndef FUNRSA_H
#define FUNRSA_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


typedef struct {
    uint32_t p, q, n, e, d; // Variáveis para armazenar n, e e d
    uint32_t  *cifra;
    size_t    tam;          // quantidade de blocos
    int       pronto;       // 1 = chaves e cifra geradas
} s_RSA;


int rsa(int opcao, s_RSA *Cifra, const char *msg, char *saida, int debug);



#endif // FUNRSA_H
