#include "FunRSA.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
  #include <windows.h>
  #include <bcrypt.h>
#endif

/* ========================================================================
 *  CONFIGURACAO
 * ========================================================================*/
#define QTD_BLOCO  130         
#define MAX_BITS   2048
#define MAX_BYTES  (MAX_BITS / 8)
#define E_PUBLICO  65537u

typedef struct {
    uint32_t d[QTD_BLOCO];      /* d[0] = limb menos significativo */
    int n;                      /* limbs em uso (0 = numero zero)  */
} bn;

/* ========================================================================
 *  ALEATORIEDADE 
 * ======================================================================== */
static int g_rng_falhou = 0;

static int random_bytes(void *buf, size_t len) {
    #ifdef _WIN32
        if (BCryptGenRandom(NULL, (PUCHAR)buf, (ULONG)len,
                            BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0) {
            g_rng_falhou = 1;
            return -1;
        }
        return 0;
    #else
        FILE *f = fopen("/dev/urandom", "rb");
        if (!f) { g_rng_falhou = 1; return -1; }
        size_t got = fread(buf, 1, len, f);
        fclose(f);
        if (got != len) { g_rng_falhou = 1; return -1; }
        return 0;
    #endif
}

/* bytes aleatorios todos diferentes de zero (exigencia do padding) */
static int random_nonzero(uint8_t *b, size_t len) {
    if (random_bytes(b, len)) return -1;
    for (size_t i = 0; i < len; i++)
        while (b[i] == 0)
            if (random_bytes(&b[i], 1)) return -1;
    return 0;
}

/* ========================================================================
 *  BIGNUM: operacoes basicas
 * ======================================================================== */
static void bn_trim(bn *a) {
    while (a->n > 0 && a->d[a->n - 1] == 0) a->n--;
}

static void bn_set_u32(bn *a, uint32_t v) {
    memset(a, 0, sizeof *a);
    if (v) { a->d[0] = v; a->n = 1; }
}

static int bn_cmp(const bn *a, const bn *b) {
    if (a->n != b->n) return a->n > b->n ? 1 : -1;
    for (int i = a->n - 1; i >= 0; i--)
        if (a->d[i] != b->d[i]) return a->d[i] > b->d[i] ? 1 : -1;
    return 0;
}

static int bn_bits(const bn *a) {
    if (a->n == 0) return 0;
    int b = (a->n - 1) * 32;
    uint32_t t = a->d[a->n - 1];
    while (t) { b++; t >>= 1; }
    return b;
}

static int bn_get_bit(const bn *a, int i) {
    return (a->d[i / 32] >> (i % 32)) & 1;
}

static void bn_set_bit(bn *a, int i) {
    a->d[i / 32] |= 1u << (i % 32);
    if (a->n < i / 32 + 1) a->n = i / 32 + 1;
}

static void bn_add(bn *r, const bn *a, const bn *b) {
    bn t; memset(&t, 0, sizeof t);
    int n = a->n > b->n ? a->n : b->n;
    uint64_t carry = 0;
    for (int i = 0; i < n; i++) {
        uint64_t s = (uint64_t)a->d[i] + b->d[i] + carry;  /* limbs acima de n sao 0 */
        t.d[i] = (uint32_t)s;
        carry = s >> 32;
    }
    t.n = n;
    if (carry) t.d[t.n++] = (uint32_t)carry;
    *r = t;
}

/* r = a - b   (exige a >= b) */
static void bn_sub(bn *r, const bn *a, const bn *b) {
    bn t; memset(&t, 0, sizeof t);
    int64_t borrow = 0;
    for (int i = 0; i < a->n; i++) {
        int64_t s = (int64_t)a->d[i] - (int64_t)b->d[i] - borrow;
        if (s < 0) { s += ((int64_t)1 << 32); borrow = 1; }
        else borrow = 0;
        t.d[i] = (uint32_t)s;
    }
    t.n = a->n;
    bn_trim(&t);
    *r = t;
}

static void bn_mul(bn *r, const bn *a, const bn *b) {
    bn t; memset(&t, 0, sizeof t);
    for (int i = 0; i < a->n; i++) {
        uint64_t carry = 0;
        for (int j = 0; j < b->n; j++) {
            uint64_t cur = t.d[i + j] + (uint64_t)a->d[i] * b->d[j] + carry;
            t.d[i + j] = (uint32_t)cur;
            carry = cur >> 32;
        }
        t.d[i + b->n] = (uint32_t)carry;
    }
    t.n = a->n + b->n;
    bn_trim(&t);
    *r = t;
}

static void bn_shr1(bn *a) {
    uint32_t carry = 0;
    for (int i = a->n - 1; i >= 0; i--) {
        uint32_t low = a->d[i] & 1;
        a->d[i] = (a->d[i] >> 1) | (carry << 31);
        carry = low;
    }
    bn_trim(a);
}

static void bn_shl1_or(bn *a, int bit) {
    uint32_t carry = (uint32_t)bit;
    for (int i = 0; i < a->n; i++) {
        uint32_t next = a->d[i] >> 31;
        a->d[i] = (a->d[i] << 1) | carry;
        carry = next;
    }
    if (carry) a->d[a->n++] = carry;
}

/* Divisao bit a bit (lenta, mas simples). Usada so em operacoes raras:
 * calcular R^2 mod n, o inverso modular e reducoes pontuais.
 * A parte pesada (modexp) usa Montgomery, abaixo. q ou r podem ser NULL. */
static void bn_divmod(bn *q, bn *r, const bn *a, const bn *b) {
    bn Q, R;
    memset(&Q, 0, sizeof Q);
    memset(&R, 0, sizeof R);
    for (int i = bn_bits(a) - 1; i >= 0; i--) {
        bn_shl1_or(&R, bn_get_bit(a, i));
        if (bn_cmp(&R, b) >= 0) {
            bn_sub(&R, &R, b);
            Q.d[i / 32] |= 1u << (i % 32);
        }
    }
    Q.n = a->n;
    bn_trim(&Q);
    if (q) *q = Q;
    if (r) *r = R;
}

/* a mod m, com m pequeno (32 bits) */
static uint32_t bn_mod_u32(const bn *a, uint32_t m) {
    uint64_t rem = 0;
    for (int i = a->n - 1; i >= 0; i--)
        rem = ((rem << 32) | a->d[i]) % m;
    return (uint32_t)rem;
}

/* numero aleatorio com no maximo 'bits' bits */
static void bn_rand_bits(bn *a, int bits) {
    memset(a, 0, sizeof *a);
    int nl = (bits + 31) / 32;
    random_bytes(a->d, (size_t)nl * 4);
    int extra = nl * 32 - bits;
    if (extra) a->d[nl - 1] &= 0xFFFFFFFFu >> extra;
    a->n = nl;
    bn_trim(a);
}

/* ========================================================================
 *  CONVERSOES (bytes e hexadecimal)
 * ======================================================================== */
static void bn_from_bytes(bn *a, const uint8_t *buf, size_t len) {
    memset(a, 0, sizeof *a);
    for (size_t i = 0; i < len; i++) {                 /* big-endian */
        size_t pos = len - 1 - i;
        a->d[pos / 4] |= (uint32_t)buf[i] << ((pos % 4) * 8);
    }
    a->n = (int)((len + 3) / 4);
    bn_trim(a);
}

static void bn_to_bytes(const bn *a, uint8_t *out, size_t len) {
    for (size_t i = 0; i < len; i++) {
        size_t pos = len - 1 - i;
        size_t limb = pos / 4;
        out[i] = limb < QTD_BLOCO ? (uint8_t)(a->d[limb] >> ((pos % 4) * 8)) : 0;
    }
}

static int hexval(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static int bn_from_hex(bn *a, const char *s) {
    size_t len = s ? strlen(s) : 0;
    if (len == 0 || len > MAX_BITS / 4) return -1;
    memset(a, 0, sizeof *a);
    for (size_t i = 0; i < len; i++) {
        int v = hexval(s[len - 1 - i]);
        if (v < 0) return -1;
        a->d[i / 8] |= (uint32_t)v << ((i % 8) * 4);
    }
    a->n = (int)((len + 7) / 8);
    bn_trim(a);
    return 0;
}

static int bn_to_hex(const bn *a, char *out, int size) {
    char tmp[QTD_BLOCO * 8 + 1];
    int pos;
    if (a->n == 0) {
        pos = sprintf(tmp, "0");
    } else {
        pos = sprintf(tmp, "%x", a->d[a->n - 1]);
        for (int i = a->n - 2; i >= 0; i--) pos += sprintf(tmp + pos, "%08x", a->d[i]);
    }
    if (pos + 1 > size) return -2;
    memcpy(out, tmp, (size_t)pos + 1);
    return pos;
}

/* ========================================================================
 *  MONTGOMERY: multiplicacao modular sem divisao
 *  Trabalha com x' = x*R mod n, onde R = 2^(32k). O produto de Montgomery
 *  calcula a*b*R^-1 mod n usando so somas e multiplicacoes.
 * ======================================================================== */
typedef struct {
    bn n;               /* modulo (impar) */
    int k;              /* limbs de n */
    uint32_t n0inv;     /* -n^-1 mod 2^32 */
    bn r2;              /* R^2 mod n */
    bn um;              /* R mod n  (o numero 1 em forma de Montgomery) */
} mont;

/* r = a*b*R^-1 mod n   (CIOS). r pode ser igual a a ou b. */
static void mont_mul(const mont *m, bn *r, const bn *a, const bn *b) {
    int k = m->k;
    uint32_t t[QTD_BLOCO + 2];
    memset(t, 0, sizeof(uint32_t) * (size_t)(k + 2));

    for (int i = 0; i < k; i++) {
        uint64_t c = 0, s;
        for (int j = 0; j < k; j++) {                  /* t += a * b[i] */
            s = (uint64_t)t[j] + (uint64_t)a->d[j] * b->d[i] + c;
            t[j] = (uint32_t)s;
            c = s >> 32;
        }
        s = (uint64_t)t[k] + c;
        t[k] = (uint32_t)s;
        t[k + 1] = (uint32_t)(s >> 32);

        uint32_t u = t[0] * m->n0inv;                  /* torna t[0] divisivel por 2^32 */
        s = (uint64_t)t[0] + (uint64_t)u * m->n.d[0];
        c = s >> 32;
        for (int j = 1; j < k; j++) {                  /* t = (t + u*n) / 2^32 */
            s = (uint64_t)t[j] + (uint64_t)u * m->n.d[j] + c;
            t[j - 1] = (uint32_t)s;
            c = s >> 32;
        }
        s = (uint64_t)t[k] + c;
        t[k - 1] = (uint32_t)s;
        t[k] = t[k + 1] + (uint32_t)(s >> 32);
    }

    bn res; memset(&res, 0, sizeof res);
    memcpy(res.d, t, sizeof(uint32_t) * (size_t)(k + 1));
    res.n = k + 1;
    bn_trim(&res);
    if (bn_cmp(&res, &m->n) >= 0) bn_sub(&res, &res, &m->n);
    *r = res;
}

static void mont_init(mont *m, const bn *n) {
    m->n = *n;
    m->k = n->n;

    /* inverso de n[0] mod 2^32 por Newton (cada passo dobra os bits corretos) */
    uint32_t x = n->d[0], inv = x;
    for (int i = 0; i < 4; i++) inv *= 2 - x * inv;
    m->n0inv = (uint32_t)(0u - inv);

    bn big; memset(&big, 0, sizeof big);
    bn_set_bit(&big, 64 * m->k);                       /* R^2 = 2^(64k) */
    bn_divmod(NULL, &m->r2, &big, n);

    bn one; bn_set_u32(&one, 1);
    mont_mul(m, &m->um, &m->r2, &one);                 /* R^2 * 1 * R^-1 = R mod n */
}

/* out = base^e, tudo em forma de Montgomery (square-and-multiply) */
static void mont_pow(const mont *m, bn *out, const bn *base_m, const bn *e) {
    bn res = m->um;
    for (int i = bn_bits(e) - 1; i >= 0; i--) {
        mont_mul(m, &res, &res, &res);
        if (bn_get_bit(e, i)) mont_mul(m, &res, &res, base_m);
    }
    *out = res;
}

/* r = base^e mod n   (n impar) */
static void bn_modexp(bn *r, const bn *base, const bn *e, const bn *n) {
    mont m; mont_init(&m, n);
    bn a = *base, am, rm, one;
    if (bn_cmp(&a, n) >= 0) bn_divmod(NULL, &a, base, n);
    mont_mul(&m, &am, &a, &m.r2);                      /* entra em Montgomery */
    mont_pow(&m, &rm, &am, e);
    bn_set_u32(&one, 1);
    mont_mul(&m, r, &rm, &one);                        /* sai de Montgomery */
}

/* ========================================================================
 *  PRIMOS: filtro por primos pequenos + Miller-Rabin
 * ======================================================================== */
#define SMALL_LIMIT 2000
static uint32_t g_pequenos[400];
static int g_n_pequenos = 0;

static void init_pequenos(void) {
    if (g_n_pequenos) return;
    for (uint32_t q = 3; q < SMALL_LIMIT; q += 2) {
        int primo = 1;
        for (uint32_t k = 3; k * k <= q; k += 2)
            if (q % k == 0) { primo = 0; break; }
        if (primo) g_pequenos[g_n_pequenos++] = q;
    }
}

static int tem_fator_pequeno(const bn *p) {
    for (int i = 0; i < g_n_pequenos; i++)
        if (bn_mod_u32(p, g_pequenos[i]) == 0) return 1;
    return 0;
}

/* 1 = provavelmente primo, 0 = composto  (n impar, grande) */
static int miller_rabin(const bn *n, int rodadas) {
    mont m; mont_init(&m, n);
    bn one, two, nm1, d, nm1_m;
    bn_set_u32(&one, 1);
    bn_set_u32(&two, 2);
    bn_sub(&nm1, n, &one);
    bn_sub(&nm1_m, n, &m.um);                          /* -1 em forma de Montgomery */

    d = nm1;
    int s = 0;
    while ((d.d[0] & 1) == 0) { bn_shr1(&d); s++; }    /* n-1 = 2^s * d */

    int bits = bn_bits(n);
    for (int r = 0; r < rodadas; r++) {
        bn a, am, x;
        do {
            bn_rand_bits(&a, bits - 1);                /* a < n-1 garantido */
            if (g_rng_falhou) return 0;
        } while (bn_cmp(&a, &two) < 0);

        mont_mul(&m, &am, &a, &m.r2);
        mont_pow(&m, &x, &am, &d);
        if (bn_cmp(&x, &m.um) == 0 || bn_cmp(&x, &nm1_m) == 0) continue;

        int composto = 1;
        for (int i = 0; i < s - 1; i++) {
            mont_mul(&m, &x, &x, &x);
            if (bn_cmp(&x, &nm1_m) == 0) { composto = 0; break; }
        }
        if (composto) return 0;
    }
    return 1;
}

/* gera primo de 'bits' bits tal que mdc(65537, p-1) = 1 */
static int gerar_primo(bn *p, int bits) {
    init_pequenos();
    for (;;) {
        bn_rand_bits(p, bits);
        if (g_rng_falhou) return -1;
        bn_set_bit(p, bits - 1);                       /* bit mais alto */
        bn_set_bit(p, bits - 2);                       /* 2o mais alto: p*q tera bits exatos */
        p->d[0] |= 1;                                  /* impar */
        if (tem_fator_pequeno(p)) continue;
        if (bn_mod_u32(p, E_PUBLICO) == 1) continue;   /* 65537 divide p-1: descarta */
        if (miller_rabin(p, 40)) return 0;
    }
}

/* ========================================================================
 *  INVERSO MODULAR (Euclides estendido, sem numeros negativos)
 *  Mantem os coeficientes sempre reduzidos mod m.
 * ======================================================================== */
static int bn_modinv(bn *res, const bn *e, const bn *m) {
    bn r0 = *m, r1 = *e, t0, t1;
    bn_set_u32(&t0, 0);
    bn_set_u32(&t1, 1);
    while (r1.n > 0) {
        bn q, rem, qt, tn, tmp;
        bn_divmod(&q, &rem, &r0, &r1);
        bn_mul(&qt, &q, &t1);
        bn_divmod(NULL, &qt, &qt, m);                  /* qt = q*t1 mod m */
        if (bn_cmp(&t0, &qt) >= 0) bn_sub(&tn, &t0, &qt);
        else { bn_add(&tmp, &t0, m); bn_sub(&tn, &tmp, &qt); }
        r0 = r1; r1 = rem;
        t0 = t1; t1 = tn;
    }
    if (!(r0.n == 1 && r0.d[0] == 1)) return -1;       /* mdc != 1: nao existe inverso */
    *res = t0;
    return 0;
}

/* ========================================================================
 *  RSA: estado (chaves) guardado dentro da DLL
 * ======================================================================== */
static bn g_n, g_e, g_d;
static int g_tem_chave = 0;

Export_macro int rsa_gerar_chaves(int bits) {
    if (bits < 512 || bits > MAX_BITS || bits % 64 != 0) return -3;
    g_rng_falhou = 0;

    bn p, q, n, pm1, qm1, phi, one, e, d;
    bn_set_u32(&one, 1);
    bn_set_u32(&e, E_PUBLICO);

    if (gerar_primo(&p, bits / 2)) return -6;
    do { if (gerar_primo(&q, bits / 2)) return -6; } while (bn_cmp(&p, &q) == 0);

    bn_mul(&n, &p, &q);                                /* n = p*q       */
    bn_sub(&pm1, &p, &one);
    bn_sub(&qm1, &q, &one);
    bn_mul(&phi, &pm1, &qm1);                          /* phi = (p-1)(q-1) */
    if (bn_modinv(&d, &e, &phi)) return -5;            /* d = e^-1 mod phi */

    g_n = n; g_e = e; g_d = d;
    g_tem_chave = 1;
    /* p, q e phi saem de escopo e nao sao guardados */
    return 0;
}

Export_macro int rsa_obter_n(char *saida, int tam) { return g_tem_chave ? bn_to_hex(&g_n, saida, tam) : -1; }
Export_macro int rsa_obter_e(char *saida, int tam) { return g_tem_chave ? bn_to_hex(&g_e, saida, tam) : -1; }
Export_macro int rsa_obter_d(char *saida, int tam) { return g_tem_chave ? bn_to_hex(&g_d, saida, tam) : -1; }

Export_macro int rsa_definir_chave_privada(const char *n_hex, const char *d_hex) {
    bn n, d;
    if (bn_from_hex(&n, n_hex) || bn_from_hex(&d, d_hex)) return -3;
    if (n.n == 0 || (n.d[0] & 1) == 0 || d.n == 0) return -3;
    if (bn_bits(&n) < 512 || bn_bits(&n) > MAX_BITS) return -3;
    g_n = n; g_d = d;
    if (!g_tem_chave) bn_set_u32(&g_e, E_PUBLICO);
    g_tem_chave = 1;
    return 0;
}

/* ========================================================================
 *  CIFRAR / DECIFRAR com padding PKCS#1 v1.5 (tipo 2)
 *    EM = 00 02 | PS (bytes aleatorios != 0, >= 8) | 00 | mensagem
 *  Mensagens maiores que (k-11) bytes sao divididas em blocos.
 *  Texto cifrado = blocos de k bytes concatenados, em hexadecimal.
 * ======================================================================== */
Export_macro int rsa_criptografar(const char *msg, char *saida, int tam_saida) {
    static const char HEX[] = "0123456789abcdef";
    if (!g_tem_chave) return -1;
    if (!msg || !saida) return -3;

    int k = (bn_bits(&g_n) + 7) / 8;
    size_t maxchunk = (size_t)k - 11;
    size_t len = strlen(msg);
    size_t blocos = len ? (len + maxchunk - 1) / maxchunk : 1;
    size_t total = blocos * (size_t)k * 2;
    if (total + 1 > (size_t)tam_saida) return -2;

    uint8_t em[MAX_BYTES], cb[MAX_BYTES];
    size_t off = 0;
    for (size_t b = 0; b < blocos; b++) {
        size_t chunk = len - off < maxchunk ? len - off : maxchunk;
        size_t ps = (size_t)k - 3 - chunk;

        em[0] = 0x00;
        em[1] = 0x02;
        if (random_nonzero(em + 2, ps)) return -6;
        em[2 + ps] = 0x00;
        memcpy(em + 3 + ps, msg + off, chunk);

        bn m, c;
        bn_from_bytes(&m, em, (size_t)k);
        bn_modexp(&c, &m, &g_e, &g_n);                 /* c = m^e mod n */
        bn_to_bytes(&c, cb, (size_t)k);

        char *dst = saida + b * (size_t)k * 2;
        for (int i = 0; i < k; i++) {
            dst[2 * i]     = HEX[cb[i] >> 4];
            dst[2 * i + 1] = HEX[cb[i] & 15];
        }
        off += chunk;
    }
    saida[total] = '\0';
    return (int)total;
}

Export_macro int rsa_descriptografar(const char *cifrado_hex, char *saida, int tam_saida) {
    if (!g_tem_chave) return -1;
    if (!cifrado_hex || !saida) return -3;

    int k = (bn_bits(&g_n) + 7) / 8;
    size_t hl = strlen(cifrado_hex);
    if (hl == 0 || hl % ((size_t)k * 2) != 0) return -3;
    size_t blocos = hl / ((size_t)k * 2);

    uint8_t cb[MAX_BYTES], em[MAX_BYTES];
    size_t pos = 0;
    for (size_t b = 0; b < blocos; b++) {
        const char *src = cifrado_hex + b * (size_t)k * 2;
        for (int i = 0; i < k; i++) {
            int hi = hexval(src[2 * i]), lo = hexval(src[2 * i + 1]);
            if (hi < 0 || lo < 0) return -3;
            cb[i] = (uint8_t)(hi * 16 + lo);
        }
        bn c, m;
        bn_from_bytes(&c, cb, (size_t)k);
        if (bn_cmp(&c, &g_n) >= 0) return -3;
        bn_modexp(&m, &c, &g_d, &g_n);                 /* m = c^d mod n */
        bn_to_bytes(&m, em, (size_t)k);

        if (em[0] != 0x00 || em[1] != 0x02) return -4; /* chave errada -> lixo */
        int i = 2;
        while (i < k && em[i] != 0x00) i++;
        if (i >= k || i < 10) return -4;               /* separador ausente / PS curto */

        size_t chunk = (size_t)(k - i - 1);
        if (pos + chunk + 1 > (size_t)tam_saida) return -2;
        memcpy(saida + pos, em + i + 1, chunk);
        pos += chunk;
    }
    saida[pos] = '\0';
    return (int)pos;
}
