#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ccn/ccn.h>
#include <ccn/uri.h>
#include <ccn/signing.h>
#include <ccn/keystore.h>
#include <ccn/hashtb.h>
#include "signed_interest.h"
#include "ccn_internal_structs.h" // To use hashtable of keystore via ccn handle - we should not need this

// NDN Signed interest demonstration code
// jburke@ucla.edu 04 May 2011
//
// Signed interest format:
// <prefix>/<sig_namespace><sigContentObject>
// where <sigContentObject> is a ContentObject with no name (it is implicitly named <prefix>) and no data
//

// ccnx API tutorials / descriptions that should be written:
// - parsedContentObject
// - using data structures associated with upcalls
//

// print_hex()
// Utility func to dump binary data to a handle.
void print_hex(FILE* fp, unsigned char* buf, int length, int  W) {
	int k;
	for (k=0; k< length; k++) {
		fprintf(fp, "%02X ", buf[k]);
		if ((k+1) % W == 0) fprintf(fp,"\n");
	}
}

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

// Our single packet handler for interest and data upcalls
enum ccn_upcall_res packet_handler(struct ccn_closure *selfp, enum ccn_upcall_kind, struct ccn_upcall_info *info);

// Application data structure passed to packet_handler in upcall
typedef struct {
	int* complete;
	int* outstanding_interests;
	struct ccn_pkey** public_key;	// We'll use this to verify interests
	struct ccn** ccn_pub;
	struct ccn** ccn_rec;
} handler_data;

// Our test content URI; the code appends a nonce.
const char* TEST_URI = "ccnx:/data_for_a_signed_interest";

int main(int argc, char** argv) {
	int res = 0;
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

    // Closure to handle upcalls
    struct ccn_closure *cl = NULL;
    cl = calloc(1, sizeof(*cl));
    cl->p = &packet_handler;
    handler_data h_data = { &complete, &outstanding_interests, &public_key, &ccn_pub, &ccn_rec};
    cl->data = &h_data;

    // Setup our one test name without signature
    struct ccn_charbuf* name;
    name = ccn_charbuf_create();
    ccn_name_from_uri(name, TEST_URI);
    ccn_name_append_nonce(name);
    fprintf(stderr, "Our name: %s/<nonce>\n", TEST_URI);

    // Set up a filter for interests in that name
    res = ccn_set_interest_filter(ccn_pub, name, cl);
    if (res < 0) {
        fprintf(stderr, "Failed to register interest (res == %d)\n", res);
        return(1);
    }

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
    res = ccn_express_interest(ccn_rec, name_signed, cl, NULL);			// TODO: AnswerOriginKind could limit to signed interest?
    outstanding_interests++;

    // Express an interest with an incorrect namespace
    struct ccn_charbuf *name_signed_copy = ccn_charbuf_create();
    ccn_charbuf_append_charbuf(name_signed_copy, name_signed);
    size_t k = name->length + 10; // Seek into the namespace part of the buffer
    name_signed_copy->buf[k] = name_signed_copy->buf[k] + 1;
    res = ccn_express_interest(ccn_rec, name_signed_copy, cl, NULL);			// TODO: AnswerOriginKind could limit to signed interest?
    outstanding_interests++;

    // Express an interest with bogus signature
    name_signed_copy = ccn_charbuf_create();
    ccn_charbuf_append_charbuf(name_signed_copy, name_signed);
    k = name->length + 30;  // Seek into the signature part of the buffer
    name_signed_copy->buf[k] = name_signed_copy->buf[k] + 1;
    res = ccn_express_interest(ccn_rec, name_signed_copy, cl, NULL);			// TODO: AnswerOriginKind could limit to signed interest?
    outstanding_interests++;

    if (res < 0) {
    	fprintf(stderr, "Error expressing interest (res == %d)\n", res);
    }
    cl = NULL;  						// freed by ccn?

    while(!complete && outstanding_interests>0) {
    	// Not sure how to handle two ccn_runs?
        ccn_run(ccn_rec, 100); /* stop if we run dry for .1 sec */
        ccn_run(ccn_pub, 100); /* stop if we run dry for .1 sec */
        fflush(stdout);
    }

    ccn_charbuf_destroy(&timestamp);
    ccn_charbuf_destroy(&keylocator);
    ccn_charbuf_destroy(&finalblockid);
    ccn_charbuf_destroy(&signed_info);
    ccn_charbuf_destroy(&name);
    ccn_charbuf_destroy(&name_signed);
    ccn_charbuf_destroy(&name_signed_copy);
    ccn_destroy(&ccn_pub);
    ccn_destroy(&ccn_rec);
    fflush(stderr);
	return(0);
}

// packet_handler()
//
enum ccn_upcall_res
packet_handler(struct ccn_closure *selfp,
                 enum ccn_upcall_kind upcall_kind,
                 struct ccn_upcall_info *info)
{
	handler_data* h_data = (handler_data*) selfp->data; // Client data returned

	fprintf(stderr, "\nUpcall from %s handle\n", (info->h==(*h_data->ccn_rec))? "receiver":"publisher");
	ccn_set_run_timeout(info->h, 0); // Return to client faster

    switch(upcall_kind) {
    case CCN_UPCALL_FINAL:
        fprintf(stderr, "CCN_UPCALL_FINAL\n");
        return (CCN_UPCALL_RESULT_OK);
    case CCN_UPCALL_INTEREST_TIMED_OUT:
        fprintf(stderr, "CCN_UPCALL_INTEREST_TIMED_OUT\n");
        (*h_data->complete) = 1; 	      // End the main loop, some sort of problem
        return (CCN_UPCALL_RESULT_OK);
    case CCN_UPCALL_CONTENT:
        fprintf(stderr, "CCN_UPCALL_CONTENT\n");
        const unsigned char* content = NULL;
        size_t content_bytes = 0;
        ccn_ref_tagged_BLOB(CCN_DTAG_Content, info->content_ccnb,
							  info->pco->offset[CCN_PCO_B_Content],
							  info->pco->offset[CCN_PCO_E_Content],
							  &content, &content_bytes);
        fprintf(stderr, "\tContent: %s\n", content);
        (*h_data->outstanding_interests)--;
        return (CCN_UPCALL_RESULT_OK);
    case CCN_UPCALL_CONTENT_UNVERIFIED:
        fprintf(stderr, "CCN_UPCALL_CONTENT_UNVERIFIED\n");
        return (CCN_UPCALL_RESULT_OK);
    case CCN_UPCALL_CONTENT_BAD:
        fprintf(stderr, "CCN_UPCALL_CONTENT_BAD\n");
        return (CCN_UPCALL_RESULT_OK);
    case CCN_UPCALL_CONSUMED_INTEREST:
        fprintf(stderr, "CCN_UPCALL_CONSUMED_INTEREST\n");
        return (CCN_UPCALL_RESULT_OK);
    case CCN_UPCALL_INTEREST:
    	fprintf(stderr, "CCN_UPCALL_INTEREST, (matched comps == %d)\n", info->matched_comps);
    	int res = 0;

    	// Verify the interest
    	res = verify_signed_interest(info->interest_ccnb, info->interest_comps,
    								 info->matched_comps, info->interest_comps->buf[0], info->interest_comps->buf[info->matched_comps],
    						 		   (*h_data->public_key));
		fprintf(stderr, "\tverify_signed_interest == %d (%s)\n", res, (res==1)?"verified":"unverified");

		// Based on the results,
		// create and send a reply using default key & algorithm for the receiving handle
		// to sign the content object.
		//
		char* reply_data = (res==1) ? "OK" : "AUTH_FAIL";	// A modest content.
		struct ccn_charbuf* reply = ccn_charbuf_create();
		struct ccn_charbuf* name = ccn_charbuf_create();
		struct ccn_signing_params sp = CCN_SIGNING_PARAMS_INIT;
	    ccn_name_init(name); // Don't need to create a new name, could just index into the buffer.
	    res = ccn_name_append_components(name, info->interest_ccnb,
	    					info->interest_comps->buf[0], info->interest_comps->buf[info->interest_comps->n - 1]);
		ccn_sign_content(info->h, reply, name, &sp, (void*)reply_data, strlen(reply_data)+1);
		res = ccn_put(info->h, reply->buf, reply->length);
		ccn_charbuf_destroy(&reply);
		ccn_charbuf_destroy(&name);
		if (res >= 0) {
			fprintf (stderr, "\tReturned Content: %s\n", reply_data);
			return (CCN_UPCALL_RESULT_INTEREST_CONSUMED);
		}
        return (CCN_UPCALL_RESULT_OK);
    }
    return (CCN_UPCALL_RESULT_ERR);
}
