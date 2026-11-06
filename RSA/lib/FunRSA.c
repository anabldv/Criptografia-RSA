#include "FunRSA.h"

#define QTD_BLOCO  130


static void gerador_de_numero(uint32_t maximo, uint32_t minimo, uint32_t *num, int debug) {
    *num = (uint32_t)((rand() % (maximo - minimo + 1)) + minimo);
        if(debug) printf("\n[DEBUG - gerador_de_numero] %u\n", *num);
}

static int verifica_primo(uint32_t num) {
    for (uint32_t i = 2; i * i <= num; i++){ 
        if (num % i == 0) return 1; 
    }
    return 0; 
}

static int verifica_p_q(uint32_t p, uint32_t q, int debug) {
    if (debug) printf("\niniciando verifica_p_q");

    if (p <= 1 || q <= 1) return 1;     // p e q devem ser maiores que 1.\n"); 
    if (verifica_primo(p) != 0) return 2; // p nao é primo
    if (verifica_primo(q) != 0) return 3; // q nao é primo
    if (p == q) return 4;                 // p e q devem ser diferentes
    
    return 0; 
}

static int calcula_bases(uint32_t p, uint32_t q, uint32_t *n, uint32_t *phi, int debug){
    if (debug) printf("\niniciando calcula_bases");

    uint64_t n64 = (uint64_t)p * q; // Calcula n = p * q
    if (n64 <= 255 || n64 > UINT32_MAX) return 1;   // precisa cobrir 1 byte e caber em 32 bits

    *n = (uint32_t)n64;
    *phi = (p - 1) * (q - 1); // Calcula totiente

    if (*n == 0 || *phi == 0) return 2; // Verifica se resultados são nulos
    
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
    uint64_t resultado = 1, b = base % mod;
    while (expoente > 0) {
        if (expoente & 1) resultado = (resultado * b) % mod;
        b = (b * b) % mod;
        expoente >>= 1;
    }
    return (uint32_t)resultado;
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

 static int chave_publica(uint32_t phi, uint32_t *e, int debug) {
    if (debug) printf("\niniciando chave_publica");

    for (int i = 0; i < 500; i++) {
        gerador_de_numero(phi, 2, e, debug); // Gera um número aleatório para e

        // Garante que e seja coprimo com phi
        if (mdc(phi, *e) == 1) {
            if (debug) printf("\n[DEBUG - 3] e = %u (tentativas: %d)\n", *e, i + 1);
            return 0; // Sucesso
        }
    }
    return 1; // limite de tentativas atingido
}

static int chave_privada(uint32_t phi, uint32_t e, uint32_t *d, int debug) {
    if (debug) printf("\niniciando chave_privada");
    // Algoritmo de Euclides estendido para encontrar d
    // O Teorema ou Identidade de Bézout afirma que 
    // para quaisquer dois números inteiros não nulos A e B 
    // existem sempre dois números inteiros X e Y (coeficientes de Bézout) 
    // tais que: A . X + B . Y = mdc(A, B)
    
    
    int64_t phi_temp = phi, e_temp = e;
    int64_t x0 = 0, x1 = 1;
    int contador = 0;

    while (e != 0) {
        int64_t quociente = phi_temp / e_temp;
        int64_t temp0 = e_temp;

        e_temp = phi_temp - quociente * e_temp;
        phi_temp = temp0;

        int64_t temp1 = x1;
        x1 = x0 - quociente * x1;
        x0 = temp1;

        //contador++;
        if ((contador++) >= 100) return 1;
        if(debug) printf("\n[DEBUG - 4] Contador: %d\n", contador);
    } 


    if(debug) printf("\n[DEBUG - 4] MDC: %lld, x: %lld\n", (long long)phi_temp, (long long)x0);
    if(debug) printf("\n[DEBUG - 4] phi: %u, e: %u, x0: %lld\n", phi, e, (long long)x0);    

    if (phi_temp != 1) return 2;
    if (x0 < 0) x0 += phi; // Ajusta d para ser positivo

    *d = (uint32_t)x0; // Atribui o valor de d


    return 0; // Sucesso
}

static int criptografar(s_RSA *c, const char *msg, int debug) {
    if (debug) printf("\niniciando criptografar");

    if (msg == NULL) return 1; // Verifica se a mensagem é nula

    size_t len = msg ? strlen(msg) : 0; // Calcula o tamanho da mensagem
    if (len == 0) return 2; // Verifica se a mensagem está vazia

    uint32_t *nova = realloc(c->cifra, len * sizeof(uint32_t));
    if (!nova) return 3; // Verifica se a alocação de memória falhou
    c->cifra = nova;

    if(debug){
        printf("\n[DEBUG - 5] Valor Decimal: "); 
        for (size_t i = 0; i < len; i++) { 
            uint32_t debug_m = (unsigned char)msg[i]; printf ("%u ", debug_m);
        }
        printf("\n");
        printf("\n[DEBUG - 5] Cifra:");     
    }

    for (size_t i = 0; i < len; i++) {
        uint32_t m = (unsigned char)msg[i]; // Converte o caractere para um número
        c->cifra[i] = (exp_modular(m, c->e, c->n));
        if(debug) printf ("%u ", c->cifra[i]);
    }

    c->tam = len; //Armazena o tamanho da mensagem criptografada
    return 0; // Sucesso
}

static int descriptografar(const s_RSA *c, char *saida, int debug) {
    if (debug) printf("\niniciando descriptografar");

    if (!c->pronto || !c->cifra || c->tam == 0) return 1; // Verifica se a mensagem criptografada está vazia

    if (c->cifra == NULL) return 2; // Verifica se a mensagem criptografada é nula
    
    if(debug){
        printf("\n[DEBUG - 6] Valor Decimal: "); 
        for (size_t i = 0; i < c->tam; i++) { 
            uint32_t m = c->cifra[i]; 
            printf ("%u ", m);
        }    
        printf("\n");
        printf("\n[DEBUG - 6] ");      
    }

    for (size_t i = 0; i < c->tam; i++) {
        uint32_t m = exp_modular(c->cifra[i], c->d, c->n);
        if (debug) printf("%u ", m);
        saida[i] = (char)m; // Converte o número de volta para caractere
    }
    saida[c->tam] = '\0';
    if (debug) printf("\n");
    return 0; // Sucesso 
} 

static int inicia_criptografia(s_RSA *Cifra, const char *msg,  int debug){
    int retorno = verifica_p_q(Cifra->p, Cifra->q, debug);
    uint32_t phi = 0;

    if (retorno != 0){
        switch (retorno){
            case 1: printf("\n[ERRO - 1] p e q precisam ser maiores que 1\n"); break;
            case 2: printf("\n[ERRO - 1] p nao e primo\n"); break;
            case 3: printf("\n[ERRO - 1] q nao e primo\n"); break;
            case 4: printf("\n[ERRO - 1] p e q nao podem ser iguais\n"); break;
        }
        return 1; // codigo de erro da verifica_p_q 
    }
    
    // Calcula n e phi com base em p e q
    if ((retorno = calcula_bases(Cifra->p, Cifra->q, &Cifra->n, &phi, debug)) != 0){
        switch(retorno){
            case 1: printf("\n[ERRO - 2] n deve ser > 255 e caber em 32 bits\n");
            case 2: printf("\n[ERRO - 2] n nao pode ser 0.\n[ERRO - 2] Valor de n: %u", Cifra->n); break;
            default: printf("\n[ERRO - 2] phi nao pode ser 0.\n[ERRO - 2] Valor de phi: %u", phi); break;
        }
        return 2; // codigo de erro da calcula_bases 
    } 

    // Calcula a chave pública (n, e) com base em p e q
    if ((retorno = chave_publica(phi, &Cifra->e, debug)) != 0){
        switch (retorno){
            case 1: printf("\n[ERRO - 3] Nao foi possivel calcular a chave PUBLICA, Contador atingiu o limite\n"); break;
        }
        return 3; // codigo de erro da chave_publica 
    }

    // Calcula a chave privada (d) com base em p, q e e
    if ((retorno = chave_privada(phi, Cifra->e, &Cifra->d, debug)) != 0){
        switch (retorno){
            case 1: printf("\n[ERRO - 4] Nao foi possivel calcular a chave PRIVADA, Contador atingiu o limite\n"); break;
            case 2: printf("\n[ERRO - 4] \"e\" nao tem inverso modulo phi\n"); break;
        }
        return 4; // codigo de erro da chave_privada 
    }

    // Chama a função de criptografia com a mensagem, n e e
    if ((retorno = criptografar(Cifra, msg, debug)) != 0){
        switch (retorno){
            case 1: printf("\n[ERRO - 5] A mensagem esta vazia\n"); break;
            case 2: printf("\n[ERRO - 5] phi esta maior que 1\n"); break;
            case 3: printf("\n[ERRO - 5] Falha de realocacao de memoria\n"); break;
        }
        return 5; // codigo de erro da criptografar 
    }

    Cifra->pronto = 1; // indica que a cifra está pronta
    return 0; // Sucesso
}

static int inicia_descriptografia(const s_RSA *Cifra, char *saida, int debug){
    int retorno = descriptografar(Cifra, saida, debug); // Chama a função de descriptografia com a mensagem, n e d

    if (retorno != 0){
        switch (retorno){
            case 1: printf("\n[ERRO - 6] A mensagem esta vazia\n"); break;
            case 2: printf("\n[ERRO - 6] A mensagem nao pode ser nula\n"); break;
            case 3: printf("\n[ERRO - 6] \n"); break;
        }
        return 6; // codigo de erro da descriptografar 
    }
    return 0; // Sucesso
}

int rsa(int opcao, s_RSA *Cifra, const char *msg, char *saida, int debug) {

    if (opcao == 1) {    

        return inicia_criptografia(Cifra, msg, debug);
 
    } else if (opcao == 2) {
        return inicia_descriptografia(Cifra, saida, debug); // Chama a função de descriptografia com a mensagem criptografada, n e d
    } else {
        return -1; 
    }
    return 0; // Sucesso

}
