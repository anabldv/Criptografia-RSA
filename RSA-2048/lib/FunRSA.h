#ifndef FUNRSA_H
#define FUNRSA_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


typedef struct {
    uint32_t p, q, n, e, d;
    uint32_t  *cifra // Variáveis para armazenar n, e e d
} RSA;


int rsa(int opcao, uint32_t *p, uint32_t *q, char *msg, uint32_t *n, uint32_t *e, uint32_t *d, char *output, int debug);



#endif // FUNRSA_H
