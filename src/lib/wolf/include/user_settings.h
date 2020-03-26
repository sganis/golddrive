#ifndef _WIN_USER_SETTINGS_H_
#define _WIN_USER_SETTINGS_H_

/* Verify this is Windows */
#ifndef _WIN32
#error This user_settings.h header is only designed for Windows
#endif

#define WOLFCRYPT_ONLY
#define WOLFSSL_KEY_GEN
#define HAVE_ECC
#define HAVE_AESGCM
#define HAVE_HASHDRBG
#define WOLFSSL_AES_COUNTER
#define WOLFSSL_SHA384
#define WOLFSSL_SHA512
#define NO_PSK
#define NO_HC128
#define NO_RC4
#define NO_RABBIT
#define NO_DSA
#define NO_MD4
#define WC_RSA_BLINDING
#define USE_FAST_MATH
#define TFM_TIMING_RESISTANT
#define ECC_TIMING_RESISTANT
#define WOLFSSL_PUBLIC_MP
#define SINGLE_THREADED
#define WOLFSSH_TERM
#define WOLFSSH_SFTP

#endif /* _WIN_USER_SETTINGS_H_ */
