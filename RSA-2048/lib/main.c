#include <stdio.h>
#include <string.h>
#include <time.h>
#include "FunRSA.h"

int main(void) {
    static char n[1024], e[64], d[1024], cifrado[8192], decifrado[4096];
    const char *msg = "Ola, RSA! Seguranca da informacao com acentuacao: coracao.";

    clock_t t0 = clock();
    int r = rsa_gerar_chaves(2048);
    printf("gerar_chaves(2048) = %d  (%.2f s)\n", r, (double)(clock() - t0) / CLOCKS_PER_SEC);
    if (r) return 1;

    rsa_obter_n(n, sizeof n);
    rsa_obter_e(e, sizeof e);
    rsa_obter_d(d, sizeof d);
    printf("n = %.64s...\ne = %s\nd = %.64s...\n\n", n, e, d);

    r = rsa_criptografar(msg, cifrado, sizeof cifrado);
    printf("criptografar -> %d chars hex\n%.80s...\n\n", r, cifrado);

    r = rsa_descriptografar(cifrado, decifrado, sizeof decifrado);
    printf("descriptografar -> %d bytes: \"%s\"\n", r, decifrado);

    /* Cifrar duas vezes a mesma mensagem da textos diferentes (padding aleatorio) */
    char c2[8192];
    rsa_criptografar(msg, c2, sizeof c2);
    printf("mesma msg, cifrado igual? %s\n", strcmp(cifrado, c2) == 0 ? "sim" : "nao");

    /* Chave errada: troca o ultimo digito de d */
    char d_errado[1024];
    strcpy(d_errado, d);
    d_errado[strlen(d_errado) - 1] = (d_errado[strlen(d_errado) - 1] == '0') ? '1' : '0';
    rsa_definir_chave_privada(n, d_errado);
    r = rsa_descriptografar(cifrado, decifrado, sizeof decifrado);
    printf("com chave errada -> codigo %d (esperado -4)\n", r);

    rsa_definir_chave_privada(n, d);
    r = rsa_descriptografar(cifrado, decifrado, sizeof decifrado);
    printf("com chave certa  -> %d bytes: \"%s\"\n", r, decifrado);
    return 0;
}