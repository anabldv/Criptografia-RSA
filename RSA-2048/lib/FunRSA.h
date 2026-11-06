#ifndef FUNRSA_H
#define FUNRSA_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
  #define Export_macro __declspec(dllexport)
#else
  #define Export_macro __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define QTD_BLOCO        130    // limbs de 32 bits (4160 bits): cabe 2^4096
#define RSA_BITS_PADRAO  2048   // tamanho de n usado quando nao e informado


// Numero grande: d[0] = limb menos significativo, n = limbs em uso (0 = numero zero)
typedef struct {
    uint32_t d[QTD_BLOCO];
    int      n;
} s_BigNum;


typedef struct {
    s_BigNum p, q, n, e, d; // Variáveis para armazenar p, q, n, e e d
    int       bits;         // tamanho de n em bits (0 = RSA_BITS_PADRAO)
    char     *cifra;        // ultima mensagem criptografada (texto no formato escolhido)
    size_t    tam;          // quantidade de blocos
    int       primos_manuais; // 1 = p e q foram informados pelo usuario
    int       chaves;       // 1 = chaves geradas
    int       pronto;       // 1 = chaves e cifra geradas
} s_RSA;


/* ==========================================================================
 *  FORMATOS (parametro "formato", em texto, sem diferenciar maiusculas):
 *      "decimal" (padrao)  | "hex" ou "hexadecimal" | "base64"
 *  NULL ou "" = decimal.
 *
 *  Cifra: Decimal = blocos separados por espaco
 *         Hex / Base64 = todos os blocos concatenados
 *  Chaves e primos: um unico numero no formato escolhido
 *
 *  CODIGOS DE RETORNO (0 = sucesso). O valor positivo indica a etapa que falhou:
 *      1 verifica_p_q   2 calcula_bases   3 chave_publica   4 chave_privada
 *      5 criptografar   6 descriptografar 7 extrai_chaves
 *     -1 opcao invalida -2 ponteiro s_RSA nulo
 *
 *  MEMORIA: toda string devolvida em "char **" e alocada com malloc dentro da
 *  DLL e deve ser liberada com rsa_liberar().
 * ========================================================================== */


// ------------------------------------------------------------- Ciclo de vida
Export_macro s_RSA *rsa_criar(int bits);        // aloca s_RSA zerado (bits = 0 -> 2048)
Export_macro void   rsa_limpa(s_RSA *Cifra);    // apaga segredos e libera Cifra->cifra
Export_macro void   rsa_destruir(s_RSA *Cifra); // rsa_limpa + free (so para rsa_criar)
Export_macro void   rsa_liberar(void *ptr);     // libera strings devolvidas pela DLL


// ------------------------------------------------------------- Chaves
// p e q informados pelo usuario. p = NULL e q = NULL -> volta a gerar aleatoriamente
Export_macro int rsa_define_primos(s_RSA *Cifra, const char *p, const char *q, const char *formato, int debug);

// Devolvem a chave publica (e, n) e a privada (d, n) no formato pedido
Export_macro int rsa_chave_publica(const s_RSA *Cifra, const char *formato, char **e, char **n, int debug);
Export_macro int rsa_chave_privada(const s_RSA *Cifra, const char *formato, char **d, char **n, int debug);


// ------------------------------------------------------------- Operacao principal
/*  opcao 1: criptografar    msg = texto claro, formato = formato de SAIDA, *saida = cifra
 *           (gera as chaves antes, se ainda nao existirem)
 *  opcao 2: descriptografar msg = cifra, formato = formato da ENTRADA, *saida = texto claro
 *  opcao 3: gerar chaves    usa p e q informados ou gera aleatorios (msg, formato e saida ignorados)
 */
Export_macro int rsa(int opcao, s_RSA *Cifra, const char *msg, const char *formato, char **saida, int debug);


#ifdef __cplusplus
}
#endif

#endif // FUNRSA_H
