/*
 * ndnInterst.c
 *
 *  Created on: Feb 8, 2011
 *      Author: nesl
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <ccn/ccn.h>
#include <ccn/uri.h>
#include <ccn/charbuf.h>

#include <string.h>

#include <ccn/signing.h>
#include <ccn/keystore.h>
#include <ccn/hashtb.h>
#include "signed_interest.h"
#include "ccn_internal_structs.h" // To use hashtable of keystore via ccn handle - we should not need this

static enum ccn_upcall_res
incoming_handler(
		struct ccn_closure* selfp,
		enum ccn_upcall_kind kind,
		struct ccn_upcall_info* info)
{
	unsigned int i;
	int res;
	const unsigned char *ptr;
	size_t length;
	printf("Received response (kind: %d)\n", kind);
	switch(kind) {
		case CCN_UPCALL_FINAL:
			printf("deregistering handler\n");
			return CCN_UPCALL_RESULT_OK;
		case CCN_UPCALL_CONTENT:
			printf("received content\n");
			res = ccn_content_get_value(info->content_ccnb, info->pco->offset[CCN_PCO_E], info->pco, &ptr, &length);
			printf("Output: ");
			for (i = 0; i < length; i++) {
				putchar(ptr[i]);
			}
			break;
		case CCN_UPCALL_INTEREST_TIMED_OUT:
			printf("request timed out - retrying\n");
			return CCN_UPCALL_RESULT_REEXPRESS;
		case CCN_UPCALL_CONTENT_UNVERIFIED:
		case CCN_UPCALL_CONTENT_BAD:
			printf("error\n");
			return CCN_UPCALL_RESULT_ERR;
		default:
			printf("Unexpected response\n");
			return CCN_UPCALL_RESULT_ERR;
	}
	return CCN_UPCALL_RESULT_OK;
}

// implement a function that reads the fixture discovery result and extracts serial number @ here
// helper functions from test_signed_interest

int build_keylocator_from_key(struct ccn_charbuf** keylocator, struct ccn_pkey* key) {
	int res = 0;
	*keylocator = ccn_charbuf_create();
	ccn_charbuf_append_tt(*keylocator, CCN_DTAG_KeyLocator, CCN_DTAG);
	ccn_charbuf_append_tt(*keylocator, CCN_DTAG_Key, CCN_DTAG);
	res = ccn_append_pubkey_blob(*keylocator, key);
	ccn_charbuf_append_closer(*keylocator); /* </Key> */
	ccn_charbuf_append_closer(*keylocator); /* </KeyLocator> */
	return (res);
}

// get_default_keys(ccn_pub, &p, &keystore, &public_key, &public_key_digest, &public_key_digest_length, &private_key);

int get_default_keys(struct ccn* h, struct ccn_signing_params* p, struct ccn_keystore** keystore, struct ccn_pkey** public_key,
		unsigned char** public_key_digest, size_t* public_key_digest_length, struct ccn_pkey** private_key) {
	// These structures are supposed to be internal to the libs but
	// there doesn't appear to be an API to deal with the keystores -
	// We could use ccn_keystore_init but seem to have to know just
	// as many implementation details. See ccnsendchunks.c
	struct hashtb_enumerator ee;
	struct hashtb_enumerator *e = &ee;
	int res = 0;
	hashtb_start(h->keystores, e);
		if (hashtb_seek(e, p->pubid, sizeof(p->pubid), 0) != HT_OLD_ENTRY) {
			fprintf(stderr,"No default keystore?\n");
			res = -1;
		} else {
			struct ccn_keystore **pk = e->data;
			(*keystore) = *pk;
			(*public_key) = (struct ccn_pkey*) ccn_keystore_public_key(*keystore);
			(*private_key) = (struct ccn_pkey*) ccn_keystore_private_key(*keystore);
			(*public_key_digest) = (unsigned char*) ccn_keystore_public_key_digest(*keystore);
			(*public_key_digest_length) = ccn_keystore_public_key_digest_length(*keystore);
		}
	hashtb_end(e);
	return(res);
}

int main(int argc, char *argv[])
{
	int res =0;
	struct ccn *ccn = NULL;
	struct ccn_charbuf *name = NULL;
	struct ccn_closure *incoming;
	//char URI[1024] = "";
	char URI[1024] = "";
	//argv[1];	
	name = ccn_charbuf_create();
	strcat(URI,argv[1]);

	res = ccn_name_from_uri(name, URI);

	if (res < 0) {
		fprintf(stderr, "bad ccn URI: %s\n", URI);
		exit(1);
	} else { 
		fprintf(stdout, "ccn URI ok: %s\n", URI);
		}
	
	//from test_signed_interest.c
	
	struct ccn* ccn_pub;
	struct ccn* ccn_rec;

	// Will hold the public/private key used for signing   
	struct ccn_pkey* public_key = NULL;
	struct ccn_pkey* private_key = NULL;

	int complete=0;
	int outstanding_interests=0;
	
	// We need two ccn handles because the same handle cannot be used
	// to answer interests it issues.
	//
	ccn_pub = ccn_create();
    if (ccn_connect(ccn_pub, NULL) == -1) {
        fprintf(stderr, "Could not connect to ccnd");
        return(1);
    }
    ccn_rec = ccn_create();
    if (ccn_connect(ccn_rec, NULL) == -1) {
        fprintf(stderr, "Could not connect to ccnd");
        return(1);
    }	
	
	// using original callback instead of struct in test_signed_interest
	incoming = calloc(1, sizeof(*incoming));
	incoming->p = incoming_handler;
	
    ccn_name_append_nonce(name);

	// Get our default keys -- Why do we have to do all this work??
    // Borrowed from ccn_client.c
    struct ccn_signing_params name_sp = CCN_SIGNING_PARAMS_INIT;
    struct ccn_signing_params p = CCN_SIGNING_PARAMS_INIT;
    struct ccn_keystore *keystore = NULL;
    struct ccn_charbuf *timestamp = NULL;
    struct ccn_charbuf *finalblockid = NULL;
    struct ccn_charbuf *keylocator = NULL;
    unsigned char* public_key_digest = NULL;
    size_t public_key_digest_length = 0;
    res = ccn_chk_signing_params(ccn_pub, &name_sp, &p, &timestamp, &finalblockid, &keylocator);
    if (res < 0)
        return(res);

    // For this test, use our default signing keys
    get_default_keys(ccn_pub, &p, &keystore,
    		&public_key, &public_key_digest, &public_key_digest_length, &private_key);

    // We'll need  a KeyLocator for our ContentObject
	// So continue borrowed code
	/* Construct a key locator containing the key itself */
    build_keylocator_from_key(&keylocator, public_key);

    // And a SignedInfo too
    struct ccn_charbuf *signed_info = ccn_charbuf_create();
    res = ccn_signed_info_create(signed_info,
    		 public_key_digest,
    		 public_key_digest_length,
			 timestamp,
			 p.type,
			 p.freshness,
			 0,  /* FinalBlockID is optional */
			 keylocator);
	
	// Sign the interest
    struct ccn_charbuf *name_signed = ccn_charbuf_create();
    sign_interest(name_signed, name, signed_info, NULL /* default digest alg */, private_key);
	
	// Express the signed interest from a different ccn handle so we get the packet
    res = ccn_express_interest(ccn_rec, name_signed, incoming, NULL);			// TODO: AnswerOriginKind could limit to signed interest?
    outstanding_interests++;
	
	fprintf(stderr,"Signed interested sent.\n");
	
    return 0;
}
