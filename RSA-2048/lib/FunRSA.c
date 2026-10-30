#include "FunRSA.h"

#define QTD_BLOCO  130


static void gerador_de_numero(uint32_t maximo, uint32_t minimo, uint32_t *num, int debug) {
    *num = (uint32_t)(rand() % (maximo - minimo + 1)) + minimo;
        if(debug == 1) printf("\n[DEBUG] %u\n", *num);
}

static int verifica_primo(uint32_t *num) {
    for (uint32_t i = 2; i * i <= *num; i++){ 
        if (*num % i == 0) return 1; 
    }
    return 0; 
}

static int verifica_p_q(uint32_t *p, uint32_t *q) {
    if (*p <= 1 || *q <= 1) return 1; // p e q devem ser maiores que 1.\n"); 
    if (verifica_primo(p) != 0) return 2; // p nao é primo
    if (verifica_primo(q) != 0) return 3; // q nao é primo
    
    return 0; 
}

static int calcula_bases(uint32_t *p, uint32_t *q, uint32_t *n, uint32_t *phi){
    *n = (*p) * (*q); // Calcula n = p * q
    *phi = (*p - 1) * (*q - 1); // Calcula totiente

    if (*n == 0 || *phi == 0) return 1; // Verifica se resultados são nulos
    
    return 0; // Sucesso
}

static uint32_t mdc(uint32_t a, uint32_t b) {
    while (b != 0) {
        uint32_t temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

static uint32_t exp_modular(uint32_t base, uint32_t expoente, uint32_t mod) {
    uint32_t resultado = 1;
    base = base % mod;
    while (expoente > 0) {
        if (expoente % 2 == 1) resultado = (uint32_t)(((uint32_t)resultado * base) % mod);
        expoente = expoente >> 1;
        base = (uint32_t)(((uint32_t)base * base) % mod);
    }
    return resultado;
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

static int chave_privada(uint32_t phi, uint32_t e, uint32_t *d, int debug) {
    // Algoritmo de Euclides estendido para encontrar d
    // O Teorema ou Identidade de Bézout afirma que 
    // para quaisquer dois números inteiros não nulos A e B 
    // existem sempre dois números inteiros X e Y (coeficientes de Bézout) 
    // tais que: A . X + B . Y = mdc(A, B)
    
    
    int64_t phi_temp = phi;
    int64_t x0 = 0, x1 = 1;
    int contador = 0;

    while (e != 0) {
        int32_t quociente = phi_temp / e;
        int32_t temp0 = e;

        e = phi_temp - quociente * e;
        phi_temp = temp0;

        int32_t temp1 = x1;
        x1 = x0 - quociente * x1;
        x0 = temp1;
        contador++;

        if (contador >= 100) return 1;
    }

    if(debug == 1) printf("\n\n[DEBUG] phi: %d, e: %d, x0: %d\n", phi, e, x0);    

    if (phi_temp != 1) return 2;
    if (x0 < 0) x0 += phi; // Ajusta d para ser positivo

    *d = (uint32_t)x0; // Atribui o valor de d


    return 0; // Sucesso
}

 static int chave_publica(uint32_t phi, uint32_t *n, uint32_t *e, int debug) {
    int contador = 0;

    do { 
        gerador_de_numero(phi, 2, e, debug); // Gera um número aleatório para e

        contador++;
        if (contador >= 500) return 1;

    } while (mdc(phi, *e) != 1); // Garante que e seja coprimo com phi

    return 0; // Sucesso
}

static int criptografar(char *msg, uint32_t n, uint32_t e, int *output, int debug) {
    if (msg == NULL) return 1; // Verifica se a mensagem é nula
    size_t len = strlen(msg);
    if (len == 0) return 1; // Verifica se a mensagem está vazia

    if(debug == 1){
        printf("\n[DEBUG] Valor Decimal: "); 
        for (size_t i = 0; i < len; i++) { 
            uint32_t m = msg[i]; printf ("%u ", m);
        }
        printf("\n");
        printf("\n[DEBUG] ");     
    }

    for (size_t i = 0; i < len; i++) {
        uint32_t m = msg[i]; // Converte o caractere para um número
        uint32_t c = (exp_modular(m, e, n));
        if(debug == 1) printf ("%u ", c);
        output[i] = c;
    }
    return 0; // Sucesso
}

static int descriptografar(int *criptografada, uint32_t n, uint32_t d, int *output, int debug) {
    if (criptografada == NULL) return -1; // Verifica se a mensagem criptografada é nula
    
    size_t len = sizeof(criptografada) / sizeof(criptografada[0]);
    if (len == 0) return -1; // Verifica se a mensagem criptografada está vazia
    
    if(debug == 1){
        printf("\n[DEBUG] Valor Decimal: "); 
        for (size_t i = 0; i < len; i++) { 
            uint32_t m = criptografada[i]; 
            printf ("%u ", m);
        }    
        printf("\n");
        printf("\n[DEBUG] ");      
    }

    for (size_t i = 0; i < len; i++) {
        uint32_t m = exp_modular(criptografada[i], d, n);

        if(debug == 1) printf ("%u ", m);
        output[i] = exp_modular(criptografada[i], d, n); // Converte o número de volta para caractere
        // uint32_t c = (uint32_t)criptografada[i];
        // uint32_t m = 1;

        //output[i] = (char)m; 
    }
    return 0; // Sucesso    
}

static int inicia_criptografia(int opcao, uint32_t *p, uint32_t *q, char *msg, uint32_t *n, uint32_t *e, uint32_t *d, int *output, int debug){
    int retorno = verifica_p_q(p, q);
    uint32_t phi = 0;

    if (retorno != 0){
        switch (retorno){
            case 1: printf("\n[ERRO] p e q precisam ser maiores que 1\n"); break;
            case 2: printf("\n[ERRO] p nao e primo\n"); break;
            case 3: printf("\n[ERRO] q nao e primo\n"); break;
        }
        return 1; // codigo de erro da verifica_p_q 
    }

    retorno = calcula_bases(p, q, n, &phi);

    if ( retorno != 0){
        switch(*n){
            case 0: printf("\n[ERRO] n nao pode ser 0.\n[ERRO] Valor de n: %u", *n); break;
            default: printf("\n[ERRO] phi nao pode ser 0.\n[ERRO] Valor de phi: %u", phi); break;
        }
        return 2; // codigo de erro da calcula_bases 
    } 

    retorno = chave_publica(phi, n, e, debug); // Calcula a chave pública (n, e) com base em p e q

    if (retorno != 0){
        switch (retorno){
            case 1: printf("\n[ERRO] Nao foi possivel calcular a chave PUBLICA, Contador atingiu o limite\n"); break;
        }
        return 3; // codigo de erro da chave_publica 
    }

    retorno = chave_privada(phi, *e, d, debug); // Calcula a chave privada (d) com base em p, q e e

    if (retorno != 0){
        switch (retorno){
            case 1: printf("\n[ERRO] Nao foi possivel calcular a chave PRIVADA, Contador atingiu o limite\n"); break;
            case 2: printf("\n[ERRO] phi está maior que 1\n"); break;
        }
        return 4; // codigo de erro da chave_privada 
    }

    retorno = criptografar(msg, *n, *e, output, debug); // Chama a função de criptografia com a mensagem, n e e

    if (retorno != 0){
        switch (retorno){
            case 1: printf("\n[ERRO] A mensagem está vazia\n"); break;
            case 2: printf("\n[ERRO] phi esta maior que 1\n"); break;
            case 3: printf("\n[ERRO] q precisa ser maior que 1\n"); break;
        }
        return 5; // codigo de erro da criptografar 
    }
    return 0; // Sucesso
}

int rsa(int opcao, s_RSA *Cifra, char *msg, int *output, int debug) {

    if (opcao == 1) {    

        return inicia_criptografia(opcao, &Cifra->p, &Cifra->q, msg, &Cifra->n, &Cifra->e, &Cifra->d, output, debug);
 
    } else if (opcao == 2) {
        descriptografar(output, Cifra->n, Cifra->d, output, debug); // Chama a função de descriptografia com a mensagem criptografada, n e d
    } else {
        return -1; 
    }
    return 0; // Sucesso

}
