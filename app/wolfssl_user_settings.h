/* wolfssl_user_settings.h
 *
 * Copyright (C) 2014-2024 wolfSSL Inc.
 *
 * This file is part of wolfSSH.
 *
 * wolfSSH is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * wolfSSH is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with wolfSSH.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef WOLFSSL_USER_SETTINGS_H
#define WOLFSSL_USER_SETTINGS_H

#ifdef __cplusplus
extern "C" {
#endif

#undef  WOLFSSL_ZEPHYR
#define WOLFSSL_ZEPHYR

#undef  TFM_TIMING_RESISTANT
#define TFM_TIMING_RESISTANT

#undef  ECC_TIMING_RESISTANT
#define ECC_TIMING_RESISTANT

#undef  WC_RSA_BLINDING
#define WC_RSA_BLINDING

#undef  HAVE_AESGCM
#define HAVE_AESGCM

/* the GCM method must be selected in THIS file - it is the effective
* settings (included from modules/lib/wolfssl/zephyr/user_settings.h via
* WOLFSSL_SETTINGS_FILE="wolfssl_user_settings.h"). The GCM defaults in
* zephyr/user_settings.h live in the #else branch of
* #ifdef WOLFSSL_SETTINGS_FILE and are never compiled. 
* without this define the build fell through to the WORD64_AVAILABLE bit-loop 
* GMULT - the real cause of the ~300 ms ws_proc avg per 16 KiB TLS record. 
* GCM_TABLE_4BIT builds a 512-byte M0 nibble table once per key at the handshake 
* (GenerateM0, RAM in tne Aes struct) and multiplies with word32ops on WC_32BIT_CPU. */
#undef GCM_TABLE_4BIT
#define GCM_TABLE_4BIT

#undef  WOLFSSL_SHA512
#define WOLFSSL_SHA512

#undef  WOLFSSL_SHA384
#define WOLFSSL_SHA384

#undef  NO_DSA
#define NO_DSA

#undef  HAVE_ECC
#define HAVE_ECC

#undef  TFM_ECC256
#define TFM_ECC256

#undef  WOLFSSL_BASE64_ENCODE
#define WOLFSSL_BASE64_ENCODE

#undef  NO_RC4
#define NO_RC4

#undef  WOLFSSL_SHA224
#define WOLFSSL_SHA224

#undef  WOLFSSL_SHA3
#define WOLFSSL_SHA3

#undef  HAVE_POLY1305
#define HAVE_POLY1305

#undef  HAVE_ONE_TIME_AUTH
#define HAVE_ONE_TIME_AUTH

#undef  HAVE_CHACHA
#define HAVE_CHACHA

#undef  HAVE_HASHDRBG
#define HAVE_HASHDRBG

#undef  HAVE_TLS_EXTENSIONS
#define HAVE_TLS_EXTENSIONS

#undef  HAVE_SUPPORTED_CURVES
#define HAVE_SUPPORTED_CURVES

#undef  HAVE_EXTENDED_MASTER
#define HAVE_EXTENDED_MASTER

#undef  NO_PSK
#define NO_PSK

#undef  NO_MD4
#define NO_MD4

//#undef  NO_PWDBASED
//#define NO_PWDBASED

#undef  USE_FAST_MATH
#define USE_FAST_MATH

#undef  WOLFSSL_NO_ASM
#define WOLFSSL_NO_ASM

#undef  WOLFSSL_X86_BUILD
#define WOLFSSL_X86_BUILD

#undef  WC_NO_ASYNC_THREADING
#define WC_NO_ASYNC_THREADING

#undef  NO_DES3
#define NO_DES3

#undef  WOLFSSL_STATIC_MEMORY
#define WOLFSSL_STATIC_MEMORY

#undef  WOLFSSL_TLS13
#define WOLFSSL_TLS13

#undef  HAVE_HKDF
#define HAVE_HKDF

#undef  WC_RSA_PSS
#define WC_RSA_PSS

#undef  HAVE_FFDHE_2048
#define HAVE_FFDHE_2048

#undef  OPENSSL_EXTRA_X509_SMALL
#define OPENSSL_EXTRA_X509_SMALL

#undef WOLFSSL_WPAS_SMALL
#define WOLFSSL_WPAS_SMALL

#undef OPENSSL_EXTRA
#define OPENSSL_EXTRA

#undef WOLFSSL_CERT_GEN
#define WOLFSSL_CERT_GEN

#undef WOLFSSL_CERT_REQ
#define WOLFSSL_CERT_REQ

#undef WOLFSSL_CERT_REQ
#define WOLFSSL_CERT_REQ

/* DEBUG_WOLFSSL disabled: the debug output goes through fprintf/vfprintf
 * which consumes several KB of the calling thread's stack on every log line.
 * With wolfSSH handshakes running on the 16K ssh_daemon_thread stack this
 * was the main contributor to its stack overflow (detected by
 * STACK_SENTINEL during the first TCP send). Re-enable only for isolated
 * debugging with a larger daemon stack. */
/* #undef DEBUG_WOLFSSL
   #define DEBUG_WOLFSSL */

/* --- wolfSSH (CONFIG_SHELL_BACKEND_WOLFSSH) support ---
 * WOLFSSL_WOLFSSH: enables wc_SSH_KDF (kdf.h/kdf.c), required by wolfSSH.
 * NO_INLINE: this fork stripped the inline implementations from misc.h's
 * !NO_INLINE branch, leaving misc.c's WC_MISC_STATIC implementations as
 * dead code. NO_INLINE turns them into external definitions inside
 * libwolfssl and exposes the WOLFSSL_LOCAL declarations (ForceZero,
 * ConstantCompare, min/max, ato32, c32toa, ...) that wolfSSH's internal.c
 * relies on. Without it internal.c fails with "implicit declaration".
 */
#undef WOLFSSL_WOLFSSH
#define WOLFSSL_WOLFSSH

#undef NO_INLINE
#define NO_INLINE

#undef HAVE_AES_ECB
#define HAVE_AES_ECB

/* WOLFSSL_AES_COUNTER: compiles wc_AesCtrSetKey/wc_AesCtrEncrypt, which
 * wolfSSH requires for the aes*-ctr ciphers (wolfssh internal.h forces
 * WOLFSSH_NO_AES_CTR when this is missing). The aes128/192/256 key sizes
 * themselves are enabled automatically by settings.h (AES_MAX_KEY_SIZE
 * defaults to 256) -- the explicit WOLFSSL_AES_256 below does NOT disable
 * the other sizes.
 *
 * HAVE_ECC384: compiles the P-384 curve, required by wolfSSH for the
 * ecdh-sha2-nistp384 KEX and the ecdsa-sha2-nistp384 host key algorithm
 * (internal.h forces WOLFSSH_NO_ECDH_SHA2_NISTP384 /
 * WOLFSSH_NO_ECDSA_SHA2_NISTP384 without WOLFSSL_SHA384 + HAVE_ECC384;
 * WOLFSSL_SHA384 is already defined above).
 *
 * IMPORTANT: WOLFSSL_AES_COUNTER adds fields to the Aes struct and
 * HAVE_ECC384 changes the ECC curve table -- touching either requires a
 * FULL pristine rebuild of everything linked against wolfSSL
 * (west build -p always), not an incremental one. */
#undef WOLFSSL_AES_COUNTER
#define WOLFSSL_AES_COUNTER

#undef HAVE_ECC384
#define HAVE_ECC384

#undef WOLFSSL_AES_256
#define WOLFSSL_AES_256

#undef WOLFSSL_AES_DIRECT
#define WOLFSSL_AES_DIRECT

#undef OPENSSL_ALL
#define OPENSSL_ALL

#undef BOOST_ASIO_USE_WOLFSSL
#define BOOST_ASIO_USE_WOLFSSL

#undef HAVE_SOCKADDR
#define HAVE_SOCKADDR

#undef HAVE_CURL
#define HAVE_CURL

#ifdef __cplusplus
}
#endif

#endif /* WOLFSSL_USER_SETTINGS_H */
