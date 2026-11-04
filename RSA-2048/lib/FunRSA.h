#ifndef FUNRSA_H
#define FUNRSA_H

#ifdef _WIN32
  #define Export_macro __declspec(dllexport)
#else
  #define Export_macro __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Codigos de retorno (valores negativos = erro):
 *   -1  nenhuma chave carregada (chame rsa_gerar_chaves antes)
 *   -2  buffer de saida pequeno demais
 *   -3  parametro/entrada invalida
 *   -4  padding invalido (chave errada ou texto cifrado corrompido)
 *   -5  nao foi possivel calcular d (nao deveria acontecer)
 *   -6  falha do gerador aleatorio do sistema
 */

/* Gera p, q, n, e, d e guarda na DLL. bits: 512..2048, multiplo de 64. */
Export_macro int rsa_gerar_chaves(int bits);

/* Exportam as chaves em hexadecimal. Retornam o tamanho da string ou erro. */
Export_macro int rsa_obter_n(char *saida, int tam_saida);
Export_macro int rsa_obter_e(char *saida, int tam_saida);
Export_macro int rsa_obter_d(char *saida, int tam_saida);

/* Define a chave privada informada pelo usuario (n e d em hex). */
Export_macro int rsa_definir_chave_privada(const char *n_hex, const char *d_hex);

/* msg (texto UTF-8, terminado em \0) -> texto cifrado em hex.
 * Retorna o tamanho da string de saida ou erro. */
Export_macro int rsa_criptografar(const char *msg, char *saida, int tam_saida);

/* texto cifrado em hex -> msg original.
 * Retorna o numero de bytes da mensagem ou erro. */
Export_macro int rsa_descriptografar(const char *cifrado_hex, char *saida, int tam_saida);

#ifdef __cplusplus
}
#endif

#endif // FUNRSA_H