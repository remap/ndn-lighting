/*
 * ccn_internal_structs.h
 *
 *  Created on: Apr 9, 2011
 *      Author: jburke

	These are necessary to get python wrappers to work.
	Borrowed from ccn_client.c


 */

#ifndef CCN_INTERNAL_STRUCTS_H_
#define CCN_INTERNAL_STRUCTS_H_

// from ccn_client.c


struct ccn {
    int sock;
    size_t outbufindex;
    struct ccn_charbuf *interestbuf;
    struct ccn_charbuf *inbuf;
    struct ccn_charbuf *outbuf;
    struct ccn_charbuf *ccndid;
    struct hashtb *interests_by_prefix;
    struct hashtb *interest_filters;
    struct ccn_skeleton_decoder decoder;
    struct ccn_indexbuf *scratch_indexbuf;
    struct hashtb *keys;    /* public keys, by pubid */
    struct hashtb *keystores;   /* unlocked private keys */
    struct ccn_charbuf *default_pubid;
    struct timeval now;
    int timeout;
    int refresh_us;
    int err;                    /* pos => errno value, neg => other */
    int errline;
    int verbose_error;
    int tap;
    int running;
};

struct expressed_interest;
struct ccn_reg_closure;

struct interests_by_prefix { /* keyed by components of name prefix */
    struct expressed_interest *list;
};

struct expressed_interest {
    int magic;                   /* for sanity checking */
    struct timeval lasttime;     /* time most recently expressed */
    struct ccn_closure *action;  /* handler for incoming content */
    unsigned char *interest_msg; /* the interest message as sent */
    size_t size;                 /* its size in bytes */
    int target;                  /* how many we want outstanding (0 or 1) */
    int outstanding;             /* number currently outstanding (0 or 1) */
    int lifetime_us;             /* interest lifetime in microseconds */
    struct ccn_charbuf *wanted_pub; /* waiting for this pub to arrive */
    struct expressed_interest *next; /* link to next in list */
};

struct interest_filter { /* keyed by components of name */
    struct ccn_closure *action;
    struct ccn_reg_closure *ccn_reg_closure;
    struct timeval expiry;       /* Expiration time */
    int flags;
};
#define CCN_FORW_WAITING_CCNDID (1<<30)

struct ccn_reg_closure {
    struct ccn_closure action;
    struct interest_filter *interest_filter;
};



// ccn_pkey isn't explicitly defined
// but in ccn_keystore.c is cast onto a pointer to EVP_PKEY
// from openssl, so we just define that here:

#include <openssl/evp.h>
// Implicit definition in ccn_keystore.c -
struct ccn_pkey {
	EVP_PKEY key;
};



// From ccn_signing.c

struct ccn_sigc {
    EVP_MD_CTX context;
    const EVP_MD *digest;
};

// from evp.h
// figured out by comparing usage in signing.c
// with int EVP_SignFinal(EVP_MD_CTX *ctx,unsigned char *sig,unsigned int *s, EVP_PKEY *pkey);

struct ccn_signature {
	unsigned char* sig;
};


// from ccn_keystore.c
#include <openssl/sha.h>

struct ccn_keystore {
    int initialized;
    EVP_PKEY *private_key;
    EVP_PKEY *public_key;
    X509 *certificate;
    ssize_t pubkey_digest_length;
    unsigned char pubkey_digest[SHA256_DIGEST_LENGTH];
};

#include <openssl/pkcs12.h>
struct ccn_certificate {
	X509 cert;
};

#endif /* CCN_INTERNAL_STRUCTS_H_ */
