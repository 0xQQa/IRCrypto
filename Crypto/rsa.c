#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include "rsa.h"

void init_tmp_rsa_struct(struct messages *tmp_messages){
    mpz_inits(tmp_messages->enc, tmp_messages->dec, NULL);
    tmp_messages->swap_buffer = calloc(1, 2 * CRYPT_MES_SIZE + 1);
}

void clear_tmp_rsa_struct(struct messages *tmp_messages) {
    mpz_clears(tmp_messages->enc, tmp_messages->dec, NULL);
    free(tmp_messages->swap_buffer);
}

void init_vars(mpz_t q, mpz_t p, mpz_t n, mpz_t qn, mpz_t e, mpz_t d) {
    mpz_inits(p, q, n, qn, e, d, NULL);
}

void clear_vars(mpz_t q, mpz_t p, mpz_t n, mpz_t qn, mpz_t e, mpz_t d) {
    mpz_clears(p, q, n, qn, e, d, NULL);
}

void create_prime(mpz_t prime){
    gmp_randstate_t     state;
    mp_bitcnt_t         seed;

    seed = time(NULL);
    gmp_randinit_mt(state);
    gmp_randseed_ui(state, seed);

    do {
        mpz_rrandomb(prime, state, RSA_SIZE);
        mpz_nextprime(prime, prime);
    } while (mpz_sizeinbase(prime, 2) < RSA_SIZE);

    gmp_randclear(state);
}

void get_q_p(mpz_t q, mpz_t p) {
    create_prime(q);
    do create_prime(p);
    while (!mpz_cmp(q, p));
}

static inline void get_n(mpz_t n ,mpz_t q, mpz_t p) {
    mpz_mul(n, q, p);
}

void get_qn(mpz_t qn ,mpz_t q, mpz_t p) {
    mpz_t tmp_p, tmp_q;

    mpz_init(tmp_p);
    mpz_init(tmp_q);

    mpz_sub_ui(tmp_p, p, 1);
    mpz_sub_ui(tmp_q, q, 1);
    mpz_mul(qn, tmp_q, tmp_p);

    mpz_clear(tmp_p);
    mpz_clear(tmp_q);
}

void get_e(mpz_t e, mpz_t qn){
    gmp_randstate_t     state;
    mp_bitcnt_t         seed;
    mpz_t               gcd;

    seed = time(NULL);
    mpz_init(gcd);
    gmp_randinit_mt(state);
    gmp_randseed_ui(state, seed);
    mpz_rrandomb(e, state,  2 * RSA_SIZE - 1);

    do {
        mpz_add_ui(e, e, 1);
        mpz_gcd(gcd, qn, e);
    } while (mpz_cmp_ui(gcd, 1) != 0);

    gmp_randclear(state);
    mpz_clear(gcd);
}

static inline void get_d(mpz_t d, mpz_t e, mpz_t qn) {
    mpz_invert(d, e, qn);
}

void encrypt(mpz_t c, mpz_t m,mpz_t e, mpz_t n) {
    mpz_powm(c, m, e, n);
}

void decrypt(mpz_t m, mpz_t c ,mpz_t d, mpz_t n) {
    mpz_powm(m, c, d, n);
}

void str_to_hex_val(void * dest, void * src, size_t size) {
    size_t i, j;

    for (i = j = 0; i < size ; i++, j += 2) sprintf((char *)&dest[j],"%02x", ((char *)src)[i]);
}

static inline unsigned char hex_to_char(unsigned char val) {
    if (val == 0) return 0;
    else if (val < 58) return val - 48;
    else return val - 87;
}

void hex_to_str_val(void * dest, void * src, size_t size) {
    size_t i;

    for (i = 0; i < size * 2; i += 2) ((unsigned char *)dest)[i / 2] = hex_to_char(((char *)src)[i]) * 16 + hex_to_char(((char *)src)[i+ 1]);
}

static inline void zeros_tmp_buffer(unsigned char *primary_buffer) {
    bzero(primary_buffer, CRYPT_MES_SIZE * 2 + 1);
}

int validate_mess_size(size_t size) {
    return (size <= CRYPT_MES_SIZE) ? 0: -1;
}

void check_parity(unsigned char *primary_buffer){
    if (strlen((char *)primary_buffer) % 2){
        memmove(&primary_buffer[1], primary_buffer, 2 * CRYPT_MES_SIZE - 1);
        ((char *)primary_buffer)[0] = '0';
    }
}

void encrypt_message(struct public_key *pub_key, struct messages *tmp_messages , void *src, size_t size) {
    zeros_tmp_buffer(tmp_messages->swap_buffer);
    str_to_hex_val(tmp_messages->swap_buffer, src, size);
    mpz_set_str(tmp_messages->dec, (const char*)tmp_messages->swap_buffer, 16);
    encrypt(tmp_messages->enc, tmp_messages->dec, pub_key->e, pub_key->n);
    mpz_get_str((char *)tmp_messages->swap_buffer, 16, tmp_messages->enc);
}

void decrypt_message(struct private_key *prv_key, struct messages *tmp_messages, void *dst, size_t size) {
    mpz_set_str(tmp_messages->enc, (const char*)tmp_messages->swap_buffer, 16);
    decrypt(tmp_messages->dec, tmp_messages->enc, prv_key->d, prv_key->n);
    zeros_tmp_buffer(tmp_messages->swap_buffer);
    mpz_get_str((char *)tmp_messages->swap_buffer, 16, tmp_messages->dec);
    check_parity(tmp_messages->swap_buffer);
    hex_to_str_val(dst, tmp_messages->swap_buffer, size);
}

void init_key_vars(struct public_key *pub_key, struct private_key *prv_key) {
    mpz_inits(prv_key->d, prv_key->n, pub_key->e, pub_key->n, NULL);
}

void clear_keys(struct public_key *pub_key, struct private_key *prv_key) {
    mpz_clears(prv_key->d, prv_key->n, pub_key->e, pub_key->n, NULL);
}

void init_keys(struct public_key *pub_key, struct private_key *prv_key) {
    mpz_t q, p, n, qn, e, d;

    init_vars(q, p, n, qn, e, d);
    init_key_vars(pub_key, prv_key);

    get_q_p(q, p);
    get_n(n, q, p);

    get_qn(qn, q, p);
    get_e(e, qn);
    get_d(d, e,qn);

    mpz_set(prv_key->n, n);
    mpz_set(prv_key->d, d);
    mpz_set(pub_key->n, n);
    mpz_set(pub_key->e, e);

    clear_vars(q, p, n, qn, e, d);
}