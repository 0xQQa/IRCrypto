//
// Created by qqa on 15.08.2020.
//

#ifndef P_COMMUNICATOR_RSA_H
#define P_COMMUNICATOR_RSA_H
#include <gmp.h>
#define CRYPT_MES_SIZE 512
#define RSA_SIZE 4 * CRYPT_MES_SIZE


struct public_key {
    mpz_t e,n;
};

struct private_key {
    mpz_t d,n;
};

struct messages {
    mpz_t   enc, dec;
    unsigned char   *swap_buffer;
};

void init_keys(struct public_key *pub_key,  struct private_key *priv_key);
void clear_keys(struct public_key *pub_key,  struct private_key *priv_key);
void init_tmp_rsa_struct(struct messages *tmp_messages);
void clear_tmp_rsa_struct(struct messages *tmp_messages);
void encrypt_message(struct public_key *pub_key, struct messages *tmp_messages, void *src, size_t size);
void decrypt_message(struct private_key *priv_key, struct messages *tmp_messages, void *dst, size_t size);
int validate_mess_size(size_t size);

#endif //P_COMMUNICATOR_RSA_H
