#include "FunRSA.h"

#ifdef _WIN32
  #include <windows.h>
  #include <bcrypt.h>        // link com -lbcrypt
#endif

#define MAX_BITS           2048
#define E_PUBLICO          65537u
#define LIMITE_PEQUENOS    1000   // filtro por divisao com numeros impares ate este valor
#define RODADAS_MR         40     // Miller-Rabin: erro <= 2^-80
#define LIMITE_TENTATIVAS  500    // tentativas para escolher e (quando 65537 nao serve)
#define LIMITE_EUCLIDES    5000   // passos maximos do Euclides estendido

#define FMT_DECIMAL  0
#define FMT_HEX      1
#define FMT_BASE64   2


//=========================================================================================  Aleatoriedade
// Gerador do sistema operacional (rand() e previsivel, nao serve para criptografia)
static int gerador_de_bytes(void *buf, size_t tam) {
#ifdef _WIN32
    return BCryptGenRandom(NULL, (PUCHAR)buf, (ULONG)tam, BCRYPT_USE_SYSTEM_PREFERRED_RNG) == 0 ? 0 : 1;
#else
    FILE *f = fopen("/dev/urandom", "rb");
    if (!f) return 1;
    size_t lidos = fread(buf, 1, tam, f);
    fclose(f);
    return lidos == tam ? 0 : 1;
#endif
}

static int gerador_de_numero(uint32_t maximo, uint32_t minimo, uint32_t *num, int debug) {
    uint32_t r;
    if (gerador_de_bytes(&r, sizeof r)) return 1;
    *num = (uint32_t)(minimo + (r % ((uint64_t)maximo - minimo + 1)));
    if (debug) printf("\n[DEBUG - gerador_de_numero] %u\n", *num);
    return 0;
}

// Apaga a memoria de verdade (o compilador nao pode otimizar fora o volatile)
static void limpa_memoria(void *ptr, size_t tam) {
    volatile uint8_t *p = (volatile uint8_t *)ptr;
    while (tam--) *p++ = 0;
}

static char *copia_texto(const char *s) {
    size_t tam = strlen(s) + 1;
    char *novo = malloc(tam);
    if (novo) memcpy(novo, s, tam);
    return novo;
}


//=========================================================================================  BigNum: operacoes basicas
static void big_ajusta(s_BigNum *a) { // remove limbs zero a esquerda
    while (a->n > 0 && a->d[a->n - 1] == 0) a->n--;
}

static void big_define_u32(s_BigNum *a, uint32_t v) {
    memset(a, 0, sizeof *a);
    if (v) { a->d[0] = v; a->n = 1; }
}

static int big_compara(const s_BigNum *a, const s_BigNum *b) { // -1 (a<b), 0 (a==b), 1 (a>b)
    if (a->n != b->n) return a->n > b->n ? 1 : -1;
    for (int i = a->n - 1; i >= 0; i--)
        if (a->d[i] != b->d[i]) return a->d[i] > b->d[i] ? 1 : -1;
    return 0;
}

static int big_bits(const s_BigNum *a) {
    if (a->n == 0) return 0;
    int b = (a->n - 1) * 32;
    uint32_t t = a->d[a->n - 1];
    while (t) { b++; t >>= 1; }
    return b;
}

static int big_bit(const s_BigNum *a, int i) {
    return (a->d[i / 32] >> (i % 32)) & 1;
}

static void big_liga_bit(s_BigNum *a, int i) {
    a->d[i / 32] |= 1u << (i % 32);
    if (a->n < i / 32 + 1) a->n = i / 32 + 1;
}

static void big_soma(s_BigNum *r, const s_BigNum *a, const s_BigNum *b) {
    s_BigNum t; memset(&t, 0, sizeof t);
    int n = a->n > b->n ? a->n : b->n;
    uint64_t vai_um = 0;
    for (int i = 0; i < n; i++) {
        uint64_t s = (uint64_t)a->d[i] + b->d[i] + vai_um; // limbs acima de n sao 0
        t.d[i] = (uint32_t)s;
        vai_um = s >> 32;
    }
    t.n = n;
    if (vai_um) t.d[t.n++] = (uint32_t)vai_um;
    *r = t; // usar temporario permite big_soma(&x, &x, &y)
}

static void big_subtrai(s_BigNum *r, const s_BigNum *a, const s_BigNum *b) { // exige a >= b
    s_BigNum t; memset(&t, 0, sizeof t);
    int64_t emprestimo = 0;
    for (int i = 0; i < a->n; i++) {
        int64_t s = (int64_t)a->d[i] - (int64_t)b->d[i] - emprestimo;
        if (s < 0) { s += ((int64_t)1 << 32); emprestimo = 1; }
        else emprestimo = 0;
        t.d[i] = (uint32_t)s;
    }
    t.n = a->n;
    big_ajusta(&t);
    *r = t;
}

static void big_multiplica(s_BigNum *r, const s_BigNum *a, const s_BigNum *b) { // metodo da escola
    s_BigNum t; memset(&t, 0, sizeof t);
    for (int i = 0; i < a->n; i++) {
        uint64_t vai_um = 0;
        for (int j = 0; j < b->n; j++) {
            uint64_t atual = t.d[i + j] + (uint64_t)a->d[i] * b->d[j] + vai_um; // nunca passa de 2^64-1
            t.d[i + j] = (uint32_t)atual;
            vai_um = atual >> 32;
        }
        t.d[i + b->n] = (uint32_t)vai_um;
    }
    t.n = a->n + b->n;
    big_ajusta(&t);
    *r = t;
}

static void big_desloca_dir(s_BigNum *a) { // a >>= 1
    uint32_t vai_um = 0;
    for (int i = a->n - 1; i >= 0; i--) {
        uint32_t baixo = a->d[i] & 1;
        a->d[i] = (a->d[i] >> 1) | (vai_um << 31);
        vai_um = baixo;
    }
    big_ajusta(a);
}

static void big_desloca_esq(s_BigNum *a, int bit) { // a = a*2 + bit
    uint32_t vai_um = (uint32_t)bit;
    for (int i = 0; i < a->n; i++) {
        uint32_t proximo = a->d[i] >> 31;
        a->d[i] = (a->d[i] << 1) | vai_um;
        vai_um = proximo;
    }
    if (vai_um) a->d[a->n++] = vai_um;
}

// Divisao bit a bit (lenta, mas simples). Usada so em operacoes raras (R^2 mod n,
// inverso modular). A parte pesada usa Montgomery. q ou r podem ser NULL.
static void big_divide(s_BigNum *q, s_BigNum *r, const s_BigNum *a, const s_BigNum *b) {
    s_BigNum Q, R;
    memset(&Q, 0, sizeof Q);
    memset(&R, 0, sizeof R);
    for (int i = big_bits(a) - 1; i >= 0; i--) {
        big_desloca_esq(&R, big_bit(a, i));
        if (big_compara(&R, b) >= 0) {
            big_subtrai(&R, &R, b);
            Q.d[i / 32] |= 1u << (i % 32);
        }
    }
    Q.n = a->n;
    big_ajusta(&Q);
    if (q) *q = Q;
    if (r) *r = R;
}

static uint32_t big_mod_u32(const s_BigNum *a, uint32_t m) { // a % m, com m de 32 bits
    uint64_t resto = 0;
    for (int i = a->n - 1; i >= 0; i--)
        resto = ((resto << 32) | a->d[i]) % m;
    return (uint32_t)resto;
}

static uint32_t big_div_u32(s_BigNum *q, const s_BigNum *a, uint32_t m) { // q = a / m, devolve o resto
    s_BigNum t = *a;
    uint64_t resto = 0;
    for (int i = a->n - 1; i >= 0; i--) {
        uint64_t atual = (resto << 32) | a->d[i];
        t.d[i] = (uint32_t)(atual / m);
        resto = atual % m;
    }
    big_ajusta(&t);
    *q = t;
    return (uint32_t)resto;
}

static void big_mul_u32(s_BigNum *a, uint32_t mul, uint32_t soma) { // a = a*mul + soma
    uint64_t vai_um = soma;
    for (int i = 0; i < a->n; i++) {
        uint64_t atual = (uint64_t)a->d[i] * mul + vai_um;
        a->d[i] = (uint32_t)atual;
        vai_um = atual >> 32;
    }
    if (vai_um) a->d[a->n++] = (uint32_t)vai_um;
}

static int big_aleatorio(s_BigNum *a, int bits) { // numero aleatorio com no maximo 'bits' bits
    memset(a, 0, sizeof *a);
    int nl = (bits + 31) / 32;
    if (gerador_de_bytes(a->d, (size_t)nl * 4)) return 1;
    int extra = nl * 32 - bits;
    if (extra) a->d[nl - 1] &= 0xFFFFFFFFu >> extra;
    a->n = nl;
    big_ajusta(a);
    return 0;
}


//=========================================================================================  BigNum: conversoes
static void big_de_bytes(s_BigNum *a, const uint8_t *buf, size_t tam) { // big-endian
    memset(a, 0, sizeof *a);
    for (size_t i = 0; i < tam; i++) {
        size_t pos = tam - 1 - i;
        a->d[pos / 4] |= (uint32_t)buf[i] << ((pos % 4) * 8);
    }
    a->n = (int)((tam + 3) / 4);
    big_ajusta(a);
}

static void big_para_bytes(const s_BigNum *a, uint8_t *saida, size_t tam) { // tamanho fixo, zeros a esquerda
    for (size_t i = 0; i < tam; i++) {
        size_t pos = tam - 1 - i;
        size_t limb = pos / 4;
        saida[i] = limb < QTD_BLOCO ? (uint8_t)(a->d[limb] >> ((pos % 4) * 8)) : 0;
    }
}

static int hexval(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static int eh_espaco(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }

// saida precisa de QTD_BLOCO * 10 + 1 bytes. Devolve o tamanho do texto
static int big_para_decimal(const s_BigNum *a, char *saida) {
    if (a->n == 0) { strcpy(saida, "0"); return 1; }
    uint32_t partes[QTD_BLOCO + 10];
    int qtd = 0;
    s_BigNum t = *a;
    while (t.n > 0) partes[qtd++] = big_div_u32(&t, &t, 1000000000u); // 9 digitos por vez
    int pos = sprintf(saida, "%u", partes[qtd - 1]);
    for (int i = qtd - 2; i >= 0; i--) pos += sprintf(saida + pos, "%09u", partes[i]);
    return pos;
}

static int big_de_decimal(s_BigNum *a, const char *s, size_t tam) { // 0 = ok, 1 = invalido
    while (tam > 1 && *s == '0') { s++; tam--; }               // zeros a esquerda
    if (tam == 0 || tam > 620) return 1;                       // 2^2048 tem 617 digitos
    memset(a, 0, sizeof *a);
    for (size_t i = 0; i < tam; i++) {
        if (s[i] < '0' || s[i] > '9') return 1;
        big_mul_u32(a, 10, (uint32_t)(s[i] - '0'));
    }
    return big_bits(a) > MAX_BITS ? 1 : 0;
}

// saida precisa de QTD_BLOCO * 8 + 1 bytes. Hexadecimal em maiusculas (como o original)
static int big_para_hex(const s_BigNum *a, char *saida) {
    if (a->n == 0) { strcpy(saida, "0"); return 1; }
    int pos = sprintf(saida, "%X", a->d[a->n - 1]);
    for (int i = a->n - 2; i >= 0; i--) pos += sprintf(saida + pos, "%08X", a->d[i]);
    return pos;
}

static int big_de_hex(s_BigNum *a, const char *s, size_t tam) { // 0 = ok, 1 = invalido
    if (tam > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) { s += 2; tam -= 2; }
    while (tam > 1 && *s == '0') { s++; tam--; }
    if (tam == 0 || tam > MAX_BITS / 4) return 1;
    memset(a, 0, sizeof *a);
    for (size_t i = 0; i < tam; i++) {
        int v = hexval(s[tam - 1 - i]);
        if (v < 0) return 1;
        a->d[i / 8] |= (uint32_t)v << ((i % 8) * 4);
    }
    a->n = (int)((tam + 7) / 8);
    big_ajusta(a);
    return 0;
}


//=========================================================================================  Base64
static const char B64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static size_t base64_tamanho(size_t n) { return 4 * ((n + 2) / 3); }

static void base64_codifica(const uint8_t *in, size_t n, char *out) {
    size_t j = 0;
    for (size_t i = 0; i < n; i += 3) {
        uint32_t v = (uint32_t)in[i] << 16;
        if (i + 1 < n) v |= (uint32_t)in[i + 1] << 8;
        if (i + 2 < n) v |= in[i + 2];
        out[j++] = B64[(v >> 18) & 63];
        out[j++] = B64[(v >> 12) & 63];
        out[j++] = (i + 1 < n) ? B64[(v >> 6) & 63] : '=';
        out[j++] = (i + 2 < n) ? B64[v & 63] : '=';
    }
    out[j] = '\0';
}

// 0 = ok, 1 = invalido. out precisa de (tam * 3 / 4 + 3) bytes. Ignora espacos
static int base64_decodifica(const char *s, size_t tam, uint8_t *out, size_t *qtd) {
    uint32_t acc = 0; int bits = 0, preench = 0; size_t n = 0, chars = 0;
    for (size_t i = 0; i < tam; i++) {
        char c = s[i];
        if (eh_espaco(c)) continue;
        chars++;
        if (c == '=') { preench++; continue; }
        if (preench) return 1;                                  // dado depois do '='
        const char *p = strchr(B64, c);
        if (!p || c == '\0') return 1;
        acc = (acc << 6) | (uint32_t)(p - B64);
        bits += 6;
        if (bits >= 8) { bits -= 8; out[n++] = (uint8_t)(acc >> bits); acc &= (1u << bits) - 1; }
    }
    if (chars == 0 || chars % 4 != 0 || preench > 2) return 1;
    *qtd = n;
    return 0;
}


//=========================================================================================  Formatos
static int igual_sem_caixa(const char *a, const char *b) {
    while (*a && *b) {
        char x = (*a >= 'A' && *a <= 'Z') ? (char)(*a + 32) : *a;
        char y = (*b >= 'A' && *b <= 'Z') ? (char)(*b + 32) : *b;
        if (x != y) return 0;
        a++; b++;
    }
    return *a == *b;
}

// NULL ou vazio = decimal (padrao). 0 = ok, 1 = formato desconhecido
static int interpreta_formato(const char *formato, int *fmt) {
    if (formato == NULL || formato[0] == '\0') { *fmt = FMT_DECIMAL; return 0; }
    if (igual_sem_caixa(formato, "decimal") || igual_sem_caixa(formato, "dec")) { *fmt = FMT_DECIMAL; return 0; }
    if (igual_sem_caixa(formato, "hexadecimal") || igual_sem_caixa(formato, "hex")) { *fmt = FMT_HEX; return 0; }
    if (igual_sem_caixa(formato, "base64") || igual_sem_caixa(formato, "b64")) { *fmt = FMT_BASE64; return 0; }
    return 1;
}

// Um numero (chave ou primo) -> texto alocado. 0 = ok, 1 = falha de alocacao
static int formata_big(const s_BigNum *a, int fmt, char **saida) {
    char tmp[QTD_BLOCO * 10 + 1];
    if (fmt == FMT_DECIMAL) big_para_decimal(a, tmp);
    else if (fmt == FMT_HEX) big_para_hex(a, tmp);
    else {
        uint8_t bytes[QTD_BLOCO * 4];
        size_t tam = (size_t)(big_bits(a) + 7) / 8;
        if (tam == 0) tam = 1;
        big_para_bytes(a, bytes, tam);
        base64_codifica(bytes, tam, tmp);
    }
    *saida = copia_texto(tmp);
    return *saida ? 0 : 1;
}

// Texto -> numero (ignora espacos nas pontas). 0 = ok, 1 = invalido
static int interpreta_big(const char *s, int fmt, s_BigNum *a) {
    if (s == NULL) return 1;
    while (eh_espaco(*s)) s++;
    size_t tam = strlen(s);
    while (tam > 0 && eh_espaco(s[tam - 1])) tam--;
    if (tam == 0) return 1;
    if (fmt == FMT_DECIMAL) return big_de_decimal(a, s, tam);
    if (fmt == FMT_HEX) return big_de_hex(a, s, tam);

    uint8_t bytes[MAX_BITS / 8 * 2 + 8];
    size_t qtd = 0;
    if (tam > sizeof bytes) return 1;
    if (base64_decodifica(s, tam, bytes, &qtd)) return 1;
    if (qtd > QTD_BLOCO * 4) return 1;
    big_de_bytes(a, bytes, qtd);
    return big_bits(a) > MAX_BITS ? 1 : 0;
}


//=========================================================================================  Montgomery
// Multiplicacao modular sem divisao. Trabalha com x' = x*R mod n, R = 2^(32k).
typedef struct {
    s_BigNum n;         // modulo (impar)
    int      k;         // limbs de n
    uint32_t n0inv;     // -n^-1 mod 2^32
    s_BigNum r2;        // R^2 mod n
    s_BigNum um;        // R mod n  (o numero 1 em forma de Montgomery)
} s_Mont;

// r = a*b*R^-1 mod n  (CIOS). r pode ser igual a a ou b
static void mont_multiplica(const s_Mont *m, s_BigNum *r, const s_BigNum *a, const s_BigNum *b) {
    int k = m->k;
    uint32_t t[QTD_BLOCO + 2];
    memset(t, 0, sizeof(uint32_t) * (size_t)(k + 2));

    for (int i = 0; i < k; i++) {
        uint64_t c = 0, s;
        for (int j = 0; j < k; j++) {                  // t += a * b[i]
            s = (uint64_t)t[j] + (uint64_t)a->d[j] * b->d[i] + c;
            t[j] = (uint32_t)s;
            c = s >> 32;
        }
        s = (uint64_t)t[k] + c;
        t[k] = (uint32_t)s;
        t[k + 1] = (uint32_t)(s >> 32);

        uint32_t u = t[0] * m->n0inv;                  // torna t[0] divisivel por 2^32
        s = (uint64_t)t[0] + (uint64_t)u * m->n.d[0];
        c = s >> 32;
        for (int j = 1; j < k; j++) {                  // t = (t + u*n) / 2^32
            s = (uint64_t)t[j] + (uint64_t)u * m->n.d[j] + c;
            t[j - 1] = (uint32_t)s;
            c = s >> 32;
        }
        s = (uint64_t)t[k] + c;
        t[k - 1] = (uint32_t)s;
        t[k] = t[k + 1] + (uint32_t)(s >> 32);
    }

    s_BigNum res; memset(&res, 0, sizeof res);
    memcpy(res.d, t, sizeof(uint32_t) * (size_t)(k + 1));
    res.n = k + 1;
    big_ajusta(&res);
    if (big_compara(&res, &m->n) >= 0) big_subtrai(&res, &res, &m->n);
    *r = res;
}

static void mont_inicia(s_Mont *m, const s_BigNum *n) { // n precisa ser impar
    m->n = *n;
    m->k = n->n;

    // inverso de n[0] mod 2^32 por Newton (cada passo dobra os bits corretos)
    uint32_t x = n->d[0], inv = x;
    for (int i = 0; i < 4; i++) inv *= 2 - x * inv;
    m->n0inv = (uint32_t)(0u - inv);

    s_BigNum grande; memset(&grande, 0, sizeof grande);
    big_liga_bit(&grande, 64 * m->k);                  // R^2 = 2^(64k)
    big_divide(NULL, &m->r2, &grande, n);

    s_BigNum um; big_define_u32(&um, 1);
    mont_multiplica(m, &m->um, &m->r2, &um);           // R^2 * 1 * R^-1 = R mod n
}

// out = base^e, tudo em forma de Montgomery (square-and-multiply)
static void mont_potencia(const s_Mont *m, s_BigNum *out, const s_BigNum *base_m, const s_BigNum *e) {
    s_BigNum res = m->um;
    for (int i = big_bits(e) - 1; i >= 0; i--) {
        mont_multiplica(m, &res, &res, &res);
        if (big_bit(e, i)) mont_multiplica(m, &res, &res, base_m);
    }
    *out = res;
}

// r = base^e mod n  (a funcao principal do RSA: c = m^e mod n e m = c^d mod n)
static void exp_modular(const s_Mont *m, s_BigNum *r, const s_BigNum *base, const s_BigNum *e) {
    s_BigNum a = *base, am, rm, um;
    if (big_compara(&a, &m->n) >= 0) big_divide(NULL, &a, base, &m->n);
    mont_multiplica(m, &am, &a, &m->r2);               // entra em Montgomery
    mont_potencia(m, &rm, &am, e);
    big_define_u32(&um, 1);
    mont_multiplica(m, r, &rm, &um);                   // sai de Montgomery
}


//=========================================================================================  Primos
static uint32_t mdc(uint32_t a, uint32_t b) {
    while (b != 0) {
        uint32_t temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

// 1 = provavelmente primo, 0 = composto (n impar, com mais de 32 bits)
static int miller_rabin(const s_BigNum *n, int rodadas) {
    s_Mont m; mont_inicia(&m, n);
    s_BigNum um, dois, nm1, d, nm1_m;
    big_define_u32(&um, 1);
    big_define_u32(&dois, 2);
    big_subtrai(&nm1, n, &um);
    big_subtrai(&nm1_m, n, &m.um);                     // -1 em forma de Montgomery

    d = nm1;
    int s = 0;
    while ((d.d[0] & 1) == 0) { // n-1 = 2^s * d
        big_desloca_dir(&d); s++; 
    } 

    int bits = big_bits(n);
    for (int r = 0; r < rodadas; r++) {
        s_BigNum a, am, x;
        do {
            if (big_aleatorio(&a, bits - 1)) return 0;  // falha do gerador: trata como composto
        } while (big_compara(&a, &dois) < 0);

        mont_multiplica(&m, &am, &a, &m.r2);
        mont_potencia(&m, &x, &am, &d);
        if (big_compara(&x, &m.um) == 0 || big_compara(&x, &nm1_m) == 0) continue;

        int composto = 1;
        for (int i = 0; i < s - 1; i++) {
            mont_multiplica(&m, &x, &x, &x);
            if (big_compara(&x, &nm1_m) == 0) { composto = 0; break; }
        }
        if (composto) return 0;
    }
    return 1;
}

// Mesma convencao do original: 0 = primo, 1 = nao e primo
static int verifica_primo(const s_BigNum *num) {
    if (num->n <= 1) {                                  // cabe em 32 bits: tentativa de divisao exata
        uint64_t v = num->n ? num->d[0] : 0;
        if (v < 2) return 1;
        for (uint64_t i = 2; i * i <= v; i++){
            if (v % i == 0) return 1;
        }
        return 0;
    }
    if ((num->d[0] & 1) == 0) return 1;
    for (uint32_t q = 3; q < LIMITE_PEQUENOS; q += 2)   // filtro barato: descarta ~90% dos candidatos
        if (big_mod_u32(num, q) == 0) return 1;
    return miller_rabin(num, RODADAS_MR) ? 0 : 1;
}

// Gera primo de 'bits' bits com mdc(65537, p-1) = 1. 0 = ok, 1 = falha do gerador
static int gera_primo(s_BigNum *p, int bits) {
    for (;;) {
        if (big_aleatorio(p, bits)) return 1;
        big_liga_bit(p, bits - 1);                      // bit mais alto: tamanho exato
        big_liga_bit(p, bits - 2);                      // 2o mais alto: p*q tera bits exatos
        p->d[0] |= 1;                                   // impar
        if (verifica_primo(p) != 0) continue;
        if (big_mod_u32(p, E_PUBLICO) == 1) continue;   // 65537 divide p-1: descarta
        return 0;
    }
}

// Sorteia p e q (bits/2 cada). 0 = ok, 1 = tamanho invalido, 2 = falha do gerador
static int gera_p_q(s_RSA *Cifra, int debug) {
    int bits = Cifra->bits ? Cifra->bits : RSA_BITS_PADRAO;
    if (debug) printf("\niniciando gera_p_q (%d bits)", bits);

    if (bits < 64 || bits > MAX_BITS || bits % 2 != 0) return 1;
    if (gera_primo(&Cifra->p, bits / 2)) return 2;
    
    do { 
        if (gera_primo(&Cifra->q, bits / 2)) return 2; 
    } while (big_compara(&Cifra->p, &Cifra->q) == 0);

    return 0;
}


//=========================================================================================  Etapas do RSA
static int verifica_p_q(const s_BigNum *p, const s_BigNum *q, int debug) {
    if (debug) printf("\niniciando verifica_p_q");

    if ((p->n == 0 || (p->n == 1 && p->d[0] <= 1)) ||
        (q->n == 0 || (q->n == 1 && q->d[0] <= 1))) return 1; // p e q devem ser maiores que 1
    if (verifica_primo(p) != 0) return 2; // p nao é primo
    if (verifica_primo(q) != 0) return 3; // q nao é primo
    if (big_compara(p, q) == 0) return 4; // p e q devem ser diferentes
    if ((p->d[0] & 1) == 0 || (q->d[0] & 1) == 0) return 5; // 2 e primo, mas n par nao serve

    return 0;
}

static int calcula_bases(const s_BigNum *p, const s_BigNum *q, s_BigNum *n, s_BigNum *phi, int debug) {
    if (debug) printf("\niniciando calcula_bases");

    s_BigNum um, pm1, qm1;
    big_define_u32(&um, 1);

    big_multiplica(n, p, q); // Calcula n = p * q
    if (big_bits(n) > MAX_BITS || (n->n == 1 && n->d[0] <= 255)) return 1; // precisa cobrir 1 byte e caber em 2048 bits

    big_subtrai(&pm1, p, &um);
    big_subtrai(&qm1, q, &um);
    big_multiplica(phi, &pm1, &qm1); // Calcula totiente

    if (n->n == 0 || phi->n == 0) return 2; // Verifica se resultados são nulos

    if (debug) printf("\n[DEBUG - 2] n com %d bits\n", big_bits(n));
    return 0; // Sucesso
}

static int chave_publica(const s_BigNum *phi, s_BigNum *e, int debug) {
    if (debug) printf("\niniciando chave_publica");

    // Expoente padrao 65537: coprimo com phi sempre que nao divide phi (65537 e primo)
    if (phi->n > 1 || phi->d[0] > E_PUBLICO) {
        if (big_mod_u32(phi, E_PUBLICO) != 0) {
            big_define_u32(e, E_PUBLICO);
            if (debug) printf("\n[DEBUG - 3] e = %u\n", E_PUBLICO);
            return 0; // Sucesso
        }
    }

    // phi pequeno (ou divisivel por 65537): sorteia e como no original
    uint32_t maximo = phi->n > 1 ? UINT32_MAX : phi->d[0] - 1; // e < phi
    if (maximo < 2) return 1;

    for (int i = 0; i < LIMITE_TENTATIVAS; i++) {
        uint32_t candidato;
        if (gerador_de_numero(maximo, 2, &candidato, 0)) return 1;

        // Garante que e seja coprimo com phi: mdc(e, phi) = mdc(e, phi mod e)
        if (mdc(candidato, big_mod_u32(phi, candidato)) == 1) {
            big_define_u32(e, candidato);
            if (debug) printf("\n[DEBUG - 3] e = %u (tentativas: %d)\n", candidato, i + 1);
            return 0; // Sucesso
        }
    }
    return 1; // limite de tentativas atingido
}

static int chave_privada(const s_BigNum *phi, const s_BigNum *e, s_BigNum *d, int debug) {
    if (debug) printf("\niniciando chave_privada");
    // Algoritmo de Euclides estendido para encontrar d (e*d = 1 mod phi).
    // Os coeficientes ficam sempre reduzidos mod phi, assim nao existem numeros negativos.

    s_BigNum phi_temp = *phi, e_temp = *e, x0, x1;
    big_define_u32(&x0, 0);
    big_define_u32(&x1, 1);
    int contador = 0;

    while (e_temp.n > 0) {
        s_BigNum quociente, resto, qx, novo, tmp;

        big_divide(&quociente, &resto, &phi_temp, &e_temp);
        big_multiplica(&qx, &quociente, &x1);
        big_divide(NULL, &qx, &qx, phi);                       // qx = quociente * x1 mod phi
        if (big_compara(&x0, &qx) >= 0) big_subtrai(&novo, &x0, &qx);
        else { big_soma(&tmp, &x0, phi); big_subtrai(&novo, &tmp, &qx); }

        phi_temp = e_temp; e_temp = resto;
        x0 = x1;           x1 = novo;

        if ((contador++) >= LIMITE_EUCLIDES) return 1;
    }

    if (debug) printf("\n[DEBUG - 4] passos do Euclides: %d\n", contador);

    if (!(phi_temp.n == 1 && phi_temp.d[0] == 1)) return 2;   // mdc != 1: e nao tem inverso

    *d = x0; // Atribui o valor de d
    return 0; // Sucesso
}


//=========================================================================================  Leitura da cifra
// Texto cifrado -> bytes (blocos de k bytes). 0 = ok, 1 = vazio, 3 = alocacao, 4 = invalido
static int le_cifra(const char *msg, int fmt, const s_BigNum *n, int k, uint8_t **buf, size_t *blocos) {
    size_t len = strlen(msg);
    *buf = NULL; *blocos = 0;

    if (fmt == FMT_DECIMAL) {
        size_t qtd = 0, i = 0;
        while (i < len) {                                      // 1a passagem: conta os numeros
            if (eh_espaco(msg[i]) || msg[i] == ',' || msg[i] == ';') { i++; continue; }
            if (msg[i] < '0' || msg[i] > '9') return 4;
            while (i < len && msg[i] >= '0' && msg[i] <= '9') i++;
            if (i < len && !(eh_espaco(msg[i]) || msg[i] == ',' || msg[i] == ';')) return 4;
            qtd++;
        }
        if (qtd == 0) return 1;
        uint8_t *dados = malloc(qtd * (size_t)k);
        if (!dados) return 3;

        size_t b = 0; i = 0;
        while (i < len) {                                      // 2a passagem: converte
            if (eh_espaco(msg[i]) || msg[i] == ',' || msg[i] == ';') { i++; continue; }
            size_t ini = i;
            while (i < len && msg[i] >= '0' && msg[i] <= '9') i++;
            s_BigNum v;
            if (big_de_decimal(&v, msg + ini, i - ini) || big_compara(&v, n) >= 0) { free(dados); return 4; }
            big_para_bytes(&v, dados + b * (size_t)k, (size_t)k);
            b++;
        }
        *buf = dados; *blocos = qtd;
        return 0;
    }

    if (fmt == FMT_HEX) {
        size_t digitos = 0;
        for (size_t i = 0; i < len; i++) {
            if (eh_espaco(msg[i])) continue;
            if (hexval(msg[i]) < 0) return 4;
            digitos++;
        }
        if (digitos == 0) return 1;
        if (digitos % ((size_t)k * 2) != 0) return 4;
        uint8_t *dados = malloc(digitos / 2);
        if (!dados) return 3;
        size_t j = 0; int alto = -1;
        for (size_t i = 0; i < len; i++) {
            if (eh_espaco(msg[i])) continue;
            int v = hexval(msg[i]);
            if (alto < 0) alto = v;
            else { dados[j++] = (uint8_t)(alto * 16 + v); alto = -1; }
        }
        *buf = dados; *blocos = digitos / 2 / (size_t)k;
        return 0;
    }

    // Base64
    uint8_t *dados = malloc(len * 3 / 4 + 3);
    if (!dados) return 3;
    size_t qtd = 0;
    if (base64_decodifica(msg, len, dados, &qtd)) { free(dados); return len == 0 ? 1 : 4; }
    if (qtd == 0) { free(dados); return 1; }
    if (qtd % (size_t)k != 0) { free(dados); return 4; }
    *buf = dados; *blocos = qtd / (size_t)k;
    return 0;
}

// Bytes cifrados -> texto no formato escolhido (alocado). NULL = falha de alocacao
static char *formata_cifra(const uint8_t *buf, size_t blocos, int k, int fmt) {
    size_t total = blocos * (size_t)k;
    char *texto;

    if (fmt == FMT_HEX) {
        texto = malloc(total * 2 + 1);
        if (!texto) return NULL;
        for (size_t i = 0; i < total; i++) sprintf(texto + 2 * i, "%02X", buf[i]);
        return texto;
    }
    if (fmt == FMT_BASE64) {
        texto = malloc(base64_tamanho(total) + 1);
        if (!texto) return NULL;
        base64_codifica(buf, total, texto);
        return texto;
    }

    // Decimal: um numero por bloco, separados por espaco
    size_t max_digitos = (size_t)k * 8 * 30103 / 100000 + 2;
    texto = malloc(blocos * (max_digitos + 1) + 1);
    if (!texto) return NULL;
    char tmp[QTD_BLOCO * 10 + 1];
    size_t pos = 0;
    for (size_t i = 0; i < blocos; i++) {
        s_BigNum v;
        big_de_bytes(&v, buf + i * (size_t)k, (size_t)k);
        int tam = big_para_decimal(&v, tmp);
        if (i) texto[pos++] = ' ';
        memcpy(texto + pos, tmp, (size_t)tam);
        pos += (size_t)tam;
    }
    texto[pos] = '\0';
    return texto;
}


//=========================================================================================  Criptografar / Descriptografar
// ECB simples: o texto e dividido em blocos de bb bytes (o maior valor que sempre fica < n).
// O ultimo bloco e completado com PKCS#7 (cada byte de enchimento vale a quantidade de bytes de enchimento).
// Cada bloco vira um numero m e e cifrado de forma independente: c = m^e mod n
static int criptografar(s_RSA *c, const char *msg, int fmt, char **saida, int debug) {
    if (debug) printf("\niniciando criptografar");

    if (msg == NULL) return 1; // Verifica se a mensagem é nula
    if (saida == NULL) return 4;

    size_t len = strlen(msg); // Calcula o tamanho da mensagem
    if (len == 0) return 2; // Verifica se a mensagem está vazia

    int k  = (big_bits(&c->n) + 7) / 8;  // bytes de cada bloco cifrado
    int bb = (big_bits(&c->n) - 1) / 8;  // bytes de texto claro por bloco
    size_t enchimento = (size_t)bb - len % (size_t)bb;
    size_t total  = len + enchimento;
    size_t blocos = total / (size_t)bb;

    uint8_t *plano   = malloc(total);
    uint8_t *cifrado = malloc(blocos * (size_t)k);
    if (!plano || !cifrado) { free(plano); free(cifrado); return 3; } // Verifica se a alocação de memória falhou

    memcpy(plano, msg, len);
    memset(plano + len, (int)enchimento, enchimento);

    if (debug) printf("\n[DEBUG - 5] bloco claro: %d bytes | bloco cifrado: %d bytes | blocos: %llu\n", bb, k, (unsigned long long)blocos);

    s_Mont ctx; mont_inicia(&ctx, &c->n);
    for (size_t i = 0; i < blocos; i++) {
        s_BigNum m, cc;
        big_de_bytes(&m, plano + i * (size_t)bb, (size_t)bb); // Converte o bloco para um número
        exp_modular(&ctx, &cc, &m, &c->e);
        big_para_bytes(&cc, cifrado + i * (size_t)k, (size_t)k);
    }

    char *texto = formata_cifra(cifrado, blocos, k, fmt);
    char *guarda = texto ? copia_texto(texto) : NULL;
    limpa_memoria(plano, total);
    free(plano); free(cifrado);
    if (!texto || !guarda) { free(texto); free(guarda); return 3; }

    free(c->cifra);
    c->cifra = guarda;  // guarda a ultima cifra (como no original)
    c->tam = blocos;    // Armazena a quantidade de blocos
    *saida = texto;
    return 0; // Sucesso
}

static int descriptografar(const s_RSA *c, const char *msg, int fmt, char **saida, int debug) {
    if (debug) printf("\niniciando descriptografar");

    if (!c->chaves) return 1; // Verifica se as chaves existem
    if (msg == NULL || saida == NULL) return 2; // Verifica se a mensagem criptografada é nula

    int k  = (big_bits(&c->n) + 7) / 8;
    int bb = (big_bits(&c->n) - 1) / 8;

    uint8_t *cifrado; size_t blocos;
    int retorno = le_cifra(msg, fmt, &c->n, k, &cifrado, &blocos);
    if (retorno != 0) return retorno == 1 ? 4 : retorno; // texto cifrado vazio conta como invalido

    size_t total = blocos * (size_t)bb;
    uint8_t *plano = malloc(total);
    if (!plano) { free(cifrado); return 3; }

    if (debug) printf("\n[DEBUG - 6] blocos: %llu\n", (unsigned long long)blocos);

    s_Mont ctx; mont_inicia(&ctx, &c->n);
    uint8_t tmp[QTD_BLOCO * 4];
    retorno = 0;
    for (size_t i = 0; i < blocos && retorno == 0; i++) {
        s_BigNum cc, m;
        big_de_bytes(&cc, cifrado + i * (size_t)k, (size_t)k);
        if (big_compara(&cc, &c->n) >= 0) { retorno = 4; break; }     // cifra precisa ser < n
        exp_modular(&ctx, &m, &cc, &c->d);                            // m = c^d mod n
        big_para_bytes(&m, tmp, (size_t)k);
        if (tmp[0] != 0) { retorno = 5; break; }                      // nao cabe no bloco: chave errada
        memcpy(plano + i * (size_t)bb, tmp + 1, (size_t)bb);
    }
    limpa_memoria(tmp, sizeof tmp);
    free(cifrado);

    size_t len = 0;
    if (retorno == 0) {                                               // valida o enchimento PKCS#7
        size_t enchimento = plano[total - 1];
        if (enchimento < 1 || enchimento > (size_t)bb || enchimento >= total) retorno = 5;
        else {
            len = total - enchimento;
            for (size_t i = len; i < total; i++) if (plano[i] != enchimento) retorno = 5;
            for (size_t i = 0; i < len; i++) if (plano[i] == 0) retorno = 5; // texto nao tem byte nulo
        }
    }

    if (retorno == 0) {
        char *texto = malloc(len + 1);
        if (!texto) retorno = 3;
        else {
            memcpy(texto, plano, len);
            texto[len] = '\0'; // converte de volta para texto
            *saida = texto;
        }
    }
    limpa_memoria(plano, total);
    free(plano);
    return retorno;
}

//===========================================================================================================================================
//============================================================  Inicializadores  ============================================================
//===========================================================================================================================================
static int inicia_chaves(s_RSA *Cifra, int debug){
    int retorno;
    s_BigNum phi;

    Cifra->chaves = 0;
    Cifra->pronto = 0;

    // Sorteia p e q só se o usuario não informar
    if (!Cifra->primos_manuais){
        if ((retorno = gera_p_q(Cifra, debug)) != 0){
            switch (retorno){
                case 1: printf("\n[ERRO - 1] Tamanho da chave invalido (use um numero par entre 64 e %d)\n", MAX_BITS); break;
                case 2: printf("\n[ERRO - 1] Falha do gerador de numeros aleatorios do sistema\n"); break;
            }
            return 1; // codigo de erro da verifica_p_q
        }
    }

    if ((retorno = verifica_p_q(&Cifra->p, &Cifra->q, debug)) != 0){
        switch (retorno){
            case 1: printf("\n[ERRO - 1] p e q precisam ser maiores que 1\n"); break;
            case 2: printf("\n[ERRO - 1] p nao e primo\n"); break;
            case 3: printf("\n[ERRO - 1] q nao e primo\n"); break;
            case 4: printf("\n[ERRO - 1] p e q nao podem ser iguais\n"); break;
            case 5: printf("\n[ERRO - 1] p e q precisam ser primos impares\n"); break;
        }
        return 1; // codigo de erro da verifica_p_q
    }

    // Calcula n e phi com base em p e q
    if ((retorno = calcula_bases(&Cifra->p, &Cifra->q, &Cifra->n, &phi, debug)) != 0){
        switch(retorno){
            case 1: printf("\n[ERRO - 2] n deve ser > 255 e caber em %d bits\n", MAX_BITS); break;
            default: printf("\n[ERRO - 2] n e phi nao podem ser 0.\n"); break;
        }
        return 2; // codigo de erro da calcula_bases
    }

    // Calcula a chave pública (n, e) com base em phi
    if ((retorno = chave_publica(&phi, &Cifra->e, debug)) != 0){
        switch (retorno){
            case 1: printf("\n[ERRO - 3] Nao foi possivel calcular a chave PUBLICA, Contador atingiu o limite\n"); break;
        }
        limpa_memoria(&phi, sizeof phi);
        return 3; // codigo de erro da chave_publica
    }

    // Calcula a chave privada (d) com base em phi e e
    if ((retorno = chave_privada(&phi, &Cifra->e, &Cifra->d, debug)) != 0){
        switch (retorno){
            case 1: printf("\n[ERRO - 4] Nao foi possivel calcular a chave PRIVADA, Contador atingiu o limite\n"); break;
            case 2: printf("\n[ERRO - 4] \"e\" nao tem inverso modulo phi\n"); break;
        }
        limpa_memoria(&phi, sizeof phi);
        return 4; // codigo de erro da chave_privada
    }

    limpa_memoria(&phi, sizeof phi); // phi revelaria d, nao e guardado
    Cifra->chaves = 1; // indica que as chaves estão prontas
    return 0; // Sucesso
}

static int inicia_criptografia(s_RSA *Cifra, const char *msg, const char *formato, char **saida, int debug){
    int fmt, retorno;

    if (interpreta_formato(formato, &fmt) != 0){
        printf("\n[ERRO - 5] Formato de saida invalido (use decimal, hex ou base64)\n");
        return 5; // codigo de erro da criptografar
    }

    // Chama a função de criptografia com a mensagem, n e e
    if ((retorno = criptografar(Cifra, msg, fmt, saida, debug)) != 0){
        switch (retorno){
            case 1: printf("\n[ERRO - 5] A mensagem nao pode ser nula\n"); break;
            case 2: printf("\n[ERRO - 5] A mensagem esta vazia\n"); break;
            case 3: printf("\n[ERRO - 5] Falha de alocacao de memoria\n"); break;
            case 4: printf("\n[ERRO - 5] O ponteiro de saida nao pode ser nulo\n"); break;
        }
        return 5; // codigo de erro da criptografar
    }

    Cifra->pronto = 1; // indica que a cifra está pronta
    return 0; // Sucesso
}

static int inicia_descriptografia(const s_RSA *Cifra, const char *msg, const char *formato, char **saida, int debug){
    int fmt, retorno;

    if (interpreta_formato(formato, &fmt) != 0){
        printf("\n[ERRO - 6] Formato de entrada invalido (use decimal, hex ou base64)\n");
        return 6; // codigo de erro da descriptografar
    }

    retorno = descriptografar(Cifra, msg, fmt, saida, debug); // Chama a função de descriptografia com a mensagem criptografada, n e d

    if (retorno != 0){
        switch (retorno){
            case 1: printf("\n[ERRO - 6] Gere as chaves primeiro\n"); break;
            case 2: printf("\n[ERRO - 6] A mensagem criptografada e a saida nao podem ser nulas\n"); break;
            case 3: printf("\n[ERRO - 6] Falha de alocacao de memoria\n"); break;
            case 4: printf("\n[ERRO - 6] Texto cifrado invalido (formato, tamanho ou valor >= n)\n"); break;
            case 5: printf("\n[ERRO - 6] Chave incorreta ou texto cifrado corrompido\n"); break;
        }
        return 6; // codigo de erro da descriptografar
    }
    return 0; // Sucesso
}


//=========================================================================================  Funcoes exportadas
Export_macro s_RSA *rsa_criar(int bits) {
    s_RSA *Cifra = calloc(1, sizeof(s_RSA));
    if (Cifra) Cifra->bits = bits;
    return Cifra;
}

Export_macro void rsa_limpa(s_RSA *Cifra) {
    if (Cifra == NULL) return;
    free(Cifra->cifra);
    limpa_memoria(Cifra, sizeof *Cifra); // apaga p, q e d da memoria
}

Export_macro void rsa_destruir(s_RSA *Cifra) {
    rsa_limpa(Cifra);
    free(Cifra);
}

Export_macro void rsa_liberar(void *ptr) {
    free(ptr);
}

Export_macro int rsa_define_primos(s_RSA *Cifra, const char *p, const char *q, const char *formato, int debug) {
    if (Cifra == NULL) return -2;
    if (debug) printf("\niniciando rsa_define_primos");

    Cifra->chaves = 0; Cifra->pronto = 0;

    if (p == NULL && q == NULL) { // volta a gerar p e q aleatoriamente
        limpa_memoria(&Cifra->p, sizeof Cifra->p);
        limpa_memoria(&Cifra->q, sizeof Cifra->q);
        Cifra->primos_manuais = 0;
        return 0;
    }

    int fmt;
    s_BigNum np, nq;
    if (p == NULL || q == NULL) { printf("\n[ERRO - 1] Informe p e q juntos\n"); return 1; }
    if (interpreta_formato(formato, &fmt) != 0) { printf("\n[ERRO - 1] Formato invalido (use decimal, hex ou base64)\n"); return 1; }
    if (interpreta_big(p, fmt, &np) != 0 || interpreta_big(q, fmt, &nq) != 0) {
        printf("\n[ERRO - 1] p ou q invalido (formato incorreto ou maior que %d bits)\n", MAX_BITS);
        return 1;
    }

    Cifra->p = np; Cifra->q = nq;
    Cifra->primos_manuais = 1;
    return 0; // a validacao de primalidade acontece em verifica_p_q
}

Export_macro int rsa_chave_publica(const s_RSA *Cifra, const char *formato, char **e, char **n, int debug) {
    if (Cifra == NULL) return -2;
    if (debug) printf("\niniciando rsa_chave_publica");

    int fmt;
    if (!Cifra->chaves) { printf("\n[ERRO - 7] Gere as chaves primeiro\n"); return 7; }
    if (e == NULL || n == NULL) { printf("\n[ERRO - 7] Os ponteiros de saida nao podem ser nulos\n"); return 7; }
    if (interpreta_formato(formato, &fmt) != 0) { printf("\n[ERRO - 7] Formato invalido (use decimal, hex ou base64)\n"); return 7; }

    *e = NULL; *n = NULL;
    if (formata_big(&Cifra->e, fmt, e) != 0 || formata_big(&Cifra->n, fmt, n) != 0) {
        free(*e); free(*n); *e = NULL; *n = NULL;
        printf("\n[ERRO - 7] Falha de alocacao de memoria\n");
        return 7;
    }
    return 0;
}

Export_macro int rsa_chave_privada(const s_RSA *Cifra, const char *formato, char **d, char **n, int debug) {
    if (Cifra == NULL) return -2;
    if (debug) printf("\niniciando rsa_chave_privada");

    int fmt;
    if (!Cifra->chaves) { printf("\n[ERRO - 7] Gere as chaves primeiro\n"); return 7; }
    if (d == NULL || n == NULL) { printf("\n[ERRO - 7] Os ponteiros de saida nao podem ser nulos\n"); return 7; }
    if (interpreta_formato(formato, &fmt) != 0) { printf("\n[ERRO - 7] Formato invalido (use decimal, hex ou base64)\n"); return 7; }

    *d = NULL; *n = NULL;
    if (formata_big(&Cifra->d, fmt, d) != 0 || formata_big(&Cifra->n, fmt, n) != 0) {
        if (*d) { limpa_memoria(*d, strlen(*d)); free(*d); }
        free(*n); *d = NULL; *n = NULL;
        printf("\n[ERRO - 7] Falha de alocacao de memoria\n");
        return 7;
    }
    return 0;
}

Export_macro int rsa(int opcao, s_RSA *Cifra, const char *msg, const char *formato, char **saida, int debug) {

    if (Cifra == NULL) return -2;

    if (opcao == 1) {

        if (!Cifra->chaves) { // gera as chaves automaticamente na primeira vez
            int retorno = inicia_chaves(Cifra, debug);
            if (retorno != 0) return retorno;
        }
        return inicia_criptografia(Cifra, msg, formato, saida, debug);

    } else if (opcao == 2) {
        return inicia_descriptografia(Cifra, msg, formato, saida, debug); // Chama a função de descriptografia com a mensagem criptografada, n e d
    } else if (opcao == 3) {
        return inicia_chaves(Cifra, debug);
    } else {
        return -1;
    }
    return 0; // Sucesso

}
