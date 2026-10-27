#include "FunRSA.h"

#define QTD_BLOCO  130


static int verifica_primo(uint32_t *num) {
    for (uint32_t i = 2; i * i <= *num; i++) {
        if (*num % i == 0) return -1; 
    }
    return 0; 
}

static int verifica_p_q(uint32_t *p, uint32_t *q) {
    if (*p <= 1 || *q <= 1) return -1; // p e q devem ser maiores que 1.\n"); 
    if (verifica_primo(p) != 0) return -1; // p nao é primo
    if (verifica_primo(q) != 0)  return -1; // q nao é primo
    
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

static int potencia (uint32_t base, uint32_t expoente) {
    uint32_t resultado = 1;
    if (expoente == 0) return 1; // Qualquer número elevado a 0 é 1

    for (uint32_t i = 0 ; i < expoente ; i++) {
        resultado = (resultado * base);
    }
    
    return resultado;
}

static int converte_hex(uint32_t *c, char *hex) {
    if (c == NULL || hex == NULL) return -1; // Verifica se os ponteiros são nulos

    sprintf(hex, "%X", *c); // Converte cada número para hexadecimal
    
    //printf("Mensagem em hexadecimal: %s\n", hex);
    return 0;
}

static int chave_privada(uint32_t *p, uint32_t *q, uint32_t *e, uint32_t *d) {
    uint32_t phi = (*p - 1) * (*q - 1); // Calcula totiente

    // Algoritmo de Euclides estendido para encontrar d
    int32_t x0 = 0, x1 = 1;
    int32_t original_phi = phi;
    int32_t original_e = *e;

    while (*e > 1) {
        int32_t q = *e / phi;
        int32_t temp = phi;

        phi = *e % phi;
        *e = temp;

        temp = x0;
        x0 = x1 - q * x0;
        x1 = temp;
    }

    if (x1 < 0) {
        x1 += original_phi; // Ajusta d para ser positivo
    }

    *d = (uint32_t)x1; // Atribui o valor de d
    return 0; // Sucesso
}

 static int chave_publica(uint32_t *p, uint32_t *q, uint32_t *n, uint32_t *e) {
    if (verifica_p_q(p, q) != 0) return -1; // p ou q nao sao primos

    *n = (*p) * (*q); // Calcula n = p * q

    uint32_t phi = (*p - 1) * (*q - 1); // Calcula totiente

    mdc(phi, 65537, e); // Calcula o máximo divisor comum (MDC) entre φ(n) e 65537
    if (*e != 1) return -1; // Verifica se o MDC é igual a 1, caso contrário, não é possível calcular a chave pública.

    
    return 0; // Sucesso
}

static int criptografar(char *msg, uint32_t n, uint32_t e, char *output) {
    if (msg == NULL) return -1; // Verifica se a mensagem é nula

    size_t len = strlen(msg);
    if (len == 0) return -1; // Verifica se a mensagem está vazia

    for (size_t i = 0; i < len; i++) {
        uint32_t m = (uint32_t)msg[i]; // Converte o caractere para um número
        uint32_t c = 1;

        // Calcula c = m^e mod n usando exponenciação modular
        for (uint32_t j = 0; j < e; j++) {
            c = (potencia(m, e)) % n;
        }
        
        output[i] = (char)c;
        //if (converte_hex(&c, &output[i])) output[i] + " "; // Armazena o resultado criptografado no output
        //printf("Mensagem criptografada: %c\n", output[i]);
    }

    return 0; // Sucesso
}

static int descriptografar(char *criptografada, uint32_t n, uint32_t d, char *output) {
    if (criptografada == NULL) return -1; // Verifica se a mensagem criptografada é nula
    
    size_t len = strlen(criptografada);
    if (len == 0) return -1; // Verifica se a mensagem criptografada está vazia

    for (size_t i = 0; i < len; i++) {
        uint32_t c = (uint32_t)criptografada[i];
        uint32_t m = 1;

        for (uint32_t j = 0; j < d; j++) {
            m = (potencia(c, d)) % n;
        }

        output[i] = (char)m; // Converte o número de volta para caractere
    }
    return 0; // Sucesso
}


int rsa(int opcao, uint32_t *p, uint32_t *q, char *msg, uint32_t *n, uint32_t *e, uint32_t *d, char *output) {
    if (verifica_p_q(p, q) != 0) return -1;
    
    chave_publica(p, q, n, e); // Calcula a chave pública (n, e) com base em p e q
    chave_privada(p, q, e, d); // Calcula a chave privada (d) com base em p, q e e

    if (opcao == 1) {

        criptografar(msg, *n, *e, output); // Chama a função de criptografia com a mensagem, n e e

    } else if (opcao == 2) {
        
        descriptografar(output, *n, *d, output); // Chama a função de descriptografia com a mensagem criptografada, n e d

    } else {

        return -1; 
    }
    return 0; // Sucesso

}
