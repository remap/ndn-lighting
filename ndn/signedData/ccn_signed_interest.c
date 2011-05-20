#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "signed_interest.h"

// The namespace we propose
const char* NS_SIGNATURE = "org.named-data.sig";
const int NS_SIGNATURE_LEN = 19;

/**
 * Sign an interest
 *
 * @param name_signed is the resulting signed name
 * @param name is the input name
 * @param signed_info is the signed info buffer
 * @param digest_algorithm is the digest algorithm as used with ccn_encode_ContentObject
 *        set to NULL for default
 * @param key is the key as used with ccn_encode_ContentObject
 *
 */

int sign_interest(struct ccn_charbuf* name_signed, struct ccn_charbuf* name,
			struct ccn_charbuf* signed_info, const char* digest_algorithm, struct ccn_pkey* key) {

	int res = 0;
	// Now assemble a signed Content object
	// Use ccn_encode_ContentObject so we can specify the key of our choice
	struct ccn_charbuf *tempContentObj = ccn_charbuf_create();
	res = ccn_encode_ContentObject(tempContentObj, name, signed_info, NULL /* no data */, 0,
								   digest_algorithm, key);
	if (res < 0) {
		fprintf(stderr, "Error building content object (res == %d)\n", res);
		return(res);
	}
	// Call replace_name to knock out the name;
	// it would be more efficient to assemble this with no name a modified ccn_encode_ContentObject() call
	// but that requires modification to the library function
	struct ccn_charbuf *empty_name = ccn_charbuf_create();
	struct ccn_charbuf *sigContentObj = ccn_charbuf_create();
	ccn_name_init(empty_name);
	// First prepend the namespace; (should this be done as a "name component"?)
	ccn_charbuf_append(sigContentObj, NS_SIGNATURE, NS_SIGNATURE_LEN);
	replace_name(sigContentObj, tempContentObj->buf, tempContentObj->length, empty_name);
	//fprintf(stderr, "replace_name == %d (%s)\n", res, (res==0)?"ok":"fail");
	/*
	// Check that we didn't break things
	struct ccn_parsed_ContentObject pco = {0};
	res = ccn_parse_ContentObject(&sigContentObj->buf[NS_SIGNATURE_LEN], sigContentObj->length - NS_SIGNATURE_LEN, &pco, NULL);
	if (res < 0) {
		fprintf(stderr, "Error parsing built content object (res == %d)\n", res);
		return(1);
	}
	*/
	ccn_charbuf_destroy(&empty_name);
	ccn_charbuf_destroy(&tempContentObj);

	// Build the final name for the interest  <prefix>/<namespace><contentObj>
	ccn_charbuf_append_charbuf(name_signed, name); // Copy the name
	ccn_name_append(name_signed, sigContentObj->buf, sigContentObj->length);  // Concatenate the new component
    // Dump the signature
    // print_hex(stderr,&(sigContentObj->buf)[NS_SIGNATURE_LEN],sigContentObj->length - NS_SIGNATURE_LEN,12);
    // fprintf(stderr,"\n");
	//
	ccn_charbuf_destroy(&sigContentObj);
	return (res);
}

/**
 * Verify a signed interest
 *
 * params are as returned in upcall info structure
 * key is what should be used to verify
 *
 * returns:
 * -1 for parsing error
 *  0 for incorrect signature / unverified
 *  1 for proper verification
 *
 */
int verify_signed_interest(const unsigned char *ccnb, const struct ccn_indexbuf *comps,
							  size_t num_comps, size_t start, size_t stop,
							  struct ccn_pkey* key) {

    fprintf(stderr,"verifying signed interest...\n");
    
    // What is info->interest_comps->n ?
    //fprintf(stderr, "Interest components %d\n", (int) info->interest_comps->n);

    unsigned char* comp;
    size_t size;
    int res;

    // Create a charbuf with the matched interest name incl nonce
	struct ccn_charbuf* name = ccn_charbuf_create();
    ccn_name_init(name);
    res = ccn_name_append_components(name, ccnb, start, stop);

    // Last component, should be the signature
    res = ccn_name_comp_get(ccnb, comps,
    						num_comps,
                            (const unsigned char**)&comp, &size);
	if (memcmp(NS_SIGNATURE, comp, NS_SIGNATURE_LEN) != 0) {
		fprintf(stderr, "debug: Last component not tagged as a signature.\n");
		return(-1);
	}
    
	// Parse our nameless, dataless content object that follows the namespace
	// and replace the name with the implicit name from the interest, so that
	// we can use the standard signature verification calls.  Could be made
	// more efficient with different library calls.
	struct ccn_charbuf* co_with_name = ccn_charbuf_create();
    unsigned char* co = &comp[NS_SIGNATURE_LEN];
    replace_name(co_with_name, co, size-NS_SIGNATURE_LEN, name);
	//fprintf(stderr, "replace_name == %d (%s)\n", res, (res==0)?"ok":"fail");

	// For now, use standard routines to verify signature
	struct ccn_parsed_ContentObject pco = {0};

    fprintf(stderr,"verifying signed interest...2\n");
    
	res = ccn_parse_ContentObject(co_with_name->buf, co_with_name->length, &pco, NULL);
	if (!res) {
		// Verify the signature against the authorized public key given to us, passed through to the handler
		res = ccn_verify_signature(co_with_name->buf, pco.offset[CCN_PCO_E], &pco, key );
	} else {
		fprintf(stderr, "debug: Constructed content object parse failed (res==%d)\n", res);
	}
    fprintf(stderr,"verifying signed interest...3\n");
	ccn_charbuf_destroy(&co_with_name);
	ccn_charbuf_destroy(&name);
	return (res);

}


// replace_name()
// Helper function to replace names in content objects
// Could build another version that works on already parsed content objects
// But as seen below it would be better to use a modified encoding call that
// didn't include the name at all.
//
int replace_name(struct ccn_charbuf* dest, unsigned char* src,  size_t src_size, struct ccn_charbuf* name) {
	struct ccn_parsed_ContentObject* pco = (struct ccn_parsed_ContentObject*) calloc(sizeof(struct ccn_parsed_ContentObject), 1);
	int res = 0;
	res = ccn_parse_ContentObject(src,src_size, pco, NULL);
    if (res < 0) {
    	free(pco);
    	return (res);
    }
	ccn_charbuf_append_tt(dest, CCN_DTAG_ContentObject, CCN_DTAG);
	ccn_charbuf_append(dest, &src[pco->offset[CCN_PCO_B_Signature]], pco->offset[CCN_PCO_E_Signature] - pco->offset[CCN_PCO_B_Signature]);
	ccn_charbuf_append_charbuf(dest, name); // Already tagged
	ccn_charbuf_append(dest, &src[pco->offset[CCN_PCO_B_SignedInfo]], pco->offset[CCN_PCO_E_SignedInfo] - pco->offset[CCN_PCO_B_SignedInfo]);
	ccnb_append_tagged_blob(dest, CCN_DTAG_Content, NULL, 0);
	ccn_charbuf_append_closer(dest);
	free(pco);
	return (0);
}

