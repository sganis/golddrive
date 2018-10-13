/*
 * This file is part of the SSH Library
 *
 * Copyright (c) 2003-2009 by Aris Adamantiadis
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/*
 * crypto.h is an include file for internal cryptographic structures of libssh
 */

#ifndef _CRYPTO_H_
#define _CRYPTO_H_

#include "config.h"

#ifdef HAVE_LIBGCRYPT
#include <gcrypt.h>
#endif
#include "libssh/wrapper.h"

#ifdef cbc_encrypt
#undef cbc_encrypt
#endif
#ifdef cbc_decrypt
#undef cbc_decrypt
#endif

#ifdef HAVE_OPENSSL_ECDH_H
#include <openssl/ecdh.h>
#endif
#include "libssh/ecdh.h"
#include "libssh/kex.h"
#include "libssh/curve25519.h"

#define DIGEST_MAX_LEN 64

enum ssh_key_exchange_e {
  /* diffie-hellman-group1-sha1 */
  SSH_KEX_DH_GROUP1_SHA1=1,
  /* diffie-hellman-group14-sha1 */
  SSH_KEX_DH_GROUP14_SHA1,
  /* ecdh-sha2-nistp256 */
  SSH_KEX_ECDH_SHA2_NISTP256,
  /* ecdh-sha2-nistp384 */
  SSH_KEX_ECDH_SHA2_NISTP384,
  /* ecdh-sha2-nistp521 */
  SSH_KEX_ECDH_SHA2_NISTP521,
  /* curve25519-sha256@libssh.org */
  SSH_KEX_CURVE25519_SHA256_LIBSSH_ORG,
  /* curve25519-sha256 */
  SSH_KEX_CURVE25519_SHA256,
  /* diffie-hellman-group16-sha512 */
  SSH_KEX_DH_GROUP16_SHA512,
  /* diffie-hellman-group18-sha512 */
  SSH_KEX_DH_GROUP18_SHA512,
};

enum ssh_cipher_e {
    SSH_NO_CIPHER=0,
    SSH_BLOWFISH_CBC,
    SSH_3DES_CBC,
    SSH_AES128_CBC,
    SSH_AES192_CBC,
    SSH_AES256_CBC,
    SSH_AES128_CTR,
    SSH_AES192_CTR,
    SSH_AES256_CTR
};

struct ssh_crypto_struct {
    bignum e,f,x,k,y;
#ifdef HAVE_ECDH
#ifdef HAVE_OPENSSL_ECC
    EC_KEY *ecdh_privkey;
#elif defined HAVE_GCRYPT_ECC
    gcry_sexp_t ecdh_privkey;
#elif defined HAVE_LIBMBEDCRYPTO
    mbedtls_ecp_keypair *ecdh_privkey;
#endif
    ssh_string ecdh_client_pubkey;
    ssh_string ecdh_server_pubkey;
#endif
#ifdef HAVE_CURVE25519
    ssh_curve25519_privkey curve25519_privkey;
    ssh_curve25519_pubkey curve25519_client_pubkey;
    ssh_curve25519_pubkey curve25519_server_pubkey;
#endif
    ssh_string dh_server_signature; /* information used by dh_handshake. */
    size_t digest_len; /* len of all the fields below */
    unsigned char *session_id;
    unsigned char *secret_hash; /* Secret hash is same as session id until re-kex */
    unsigned char *encryptIV;
    unsigned char *decryptIV;
    unsigned char *decryptkey;
    unsigned char *encryptkey;
    unsigned char *encryptMAC;
    unsigned char *decryptMAC;
    unsigned char hmacbuf[DIGEST_MAX_LEN];
    struct ssh_cipher_struct *in_cipher, *out_cipher; /* the cipher structures/objects */
    enum ssh_hmac_e in_hmac, out_hmac; /* the MAC algorithms used */

    ssh_key server_pubkey;
    int do_compress_out; /* idem */
    int do_compress_in; /* don't set them, set the option instead */
    int delayed_compress_in; /* Use of zlib@openssh.org */
    int delayed_compress_out;
    void *compress_out_ctx; /* don't touch it */
    void *compress_in_ctx; /* really, don't */
    /* kex sent by server, client, and mutually elected methods */
    struct ssh_kex_struct server_kex;
    struct ssh_kex_struct client_kex;
    char *kex_methods[SSH_KEX_METHODS];
    enum ssh_key_exchange_e kex_type;
    enum ssh_mac_e mac_type; /* Mac operations to use for key gen */
};

struct ssh_cipher_struct {
    const char *name; /* ssh name of the algorithm */
    unsigned int blocksize; /* blocksize of the algo */
    enum ssh_cipher_e ciphertype;
    uint32_t lenfield_blocksize; /* blocksize of the packet length field */
    size_t keylen; /* length of the key structure */
#ifdef HAVE_LIBGCRYPT
    gcry_cipher_hd_t *key;
#elif defined HAVE_LIBCRYPTO
    struct ssh_3des_key_schedule *des3_key;
    struct ssh_aes_key_schedule *aes_key;
    const EVP_CIPHER *cipher;
    EVP_CIPHER_CTX *ctx;
#elif defined HAVE_LIBMBEDCRYPTO
    mbedtls_cipher_context_t encrypt_ctx;
    mbedtls_cipher_context_t decrypt_ctx;
    mbedtls_cipher_type_t type;
#endif
    struct chacha20_poly1305_keysched *chacha20_schedule;
    unsigned int keysize; /* bytes of key used. != keylen */
    size_t tag_size; /* overhead required for tag */
    /* sets the new key for immediate use */
    int (*set_encrypt_key)(struct ssh_cipher_struct *cipher, void *key, void *IV);
    int (*set_decrypt_key)(struct ssh_cipher_struct *cipher, void *key, void *IV);
    void (*encrypt)(struct ssh_cipher_struct *cipher, void *in, void *out,
        unsigned long len);
    void (*decrypt)(struct ssh_cipher_struct *cipher, void *in, void *out,
        unsigned long len);
    void (*aead_encrypt)(struct ssh_cipher_struct *cipher, void *in, void *out,
        size_t len, uint8_t *mac, uint64_t seq);
    int (*aead_decrypt_length)(struct ssh_cipher_struct *cipher, void *in,
        uint8_t *out, size_t len, uint64_t seq);
    int (*aead_decrypt)(struct ssh_cipher_struct *cipher, void *complete_packet, uint8_t *out,
        size_t encrypted_size, uint64_t seq);
    void (*cleanup)(struct ssh_cipher_struct *cipher);
};

const struct ssh_cipher_struct *ssh_get_chacha20poly1305_cipher(void);

#endif /* _CRYPTO_H_ */
