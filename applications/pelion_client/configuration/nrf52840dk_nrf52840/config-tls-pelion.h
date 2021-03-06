/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CONFIG_TLS_PELION_H
#define CONFIG_TLS_PELION_H

/* Contains mbedtls configuration options that are not configurable
 * with Kconfig and are not already enabled by default.
 */
#define MBEDTLS_ENTROPY_HARDWARE_ALT

#define MBEDTLS_PK_WRITE_C

#define MBEDTLS_X509_CHECK_KEY_USAGE
#define MBEDTLS_X509_CHECK_EXTENDED_KEY_USAGE
#define MBEDTLS_X509_CRL_PARSE_C
#define MBEDTLS_X509_CSR_PARSE_C
#define MBEDTLS_X509_CREATE_C
#define MBEDTLS_X509_CSR_WRITE_C

#define MBEDTLS_SSL_ALL_ALERT_MESSAGES
#define MBEDTLS_SSL_RENEGOTIATION
#define MBEDTLS_SSL_ALPN
#define MBEDTLS_SSL_DTLS_CLIENT_PORT_REUSE
#define MBEDTLS_SSL_DTLS_BADMAC_LIMIT
#define MBEDTLS_SSL_SESSION_TICKETS
#define MBEDTLS_SSL_SERVER_NAME_INDICATION
#define MBEDTLS_SSL_CACHE_C
#define MBEDTLS_SSL_TICKET_C
#define MBEDTLS_SSL_CIPHERSUITES MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_CCM_8,		\
				 MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256,	\
				 MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384,	\
				 MBEDTLS_TLS_PSK_WITH_AES_128_CCM_8,			\
				 MBEDTLS_TLS_PSK_WITH_AES_256_CCM_8,			\
				 MBEDTLS_TLS_ECDHE_ECDSA_WITH_ARIA_128_GCM_SHA256
#define MBEDTLS_SSL_CONTEXT_SERIALIZATION
#define MBEDTLS_SSL_DTLS_CONNECTION_ID

#define MBEDTLS_ARIA_C

#endif /* CONFIG_TLS_PELION_H */
