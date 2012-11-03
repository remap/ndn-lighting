
#ifndef SIGNED_INTEREST_H_
#define SIGNED_INTEREST_H_

#include <ccn/ccn.h>
#include <ccn/signing.h>

int sign_interest(struct ccn_charbuf* name_signed, struct ccn_charbuf* name,
			struct ccn_charbuf* signed_info, const char* digest_algorithm, struct ccn_pkey* key);

// Returns 1 if successfully verified
//
int verify_signed_interest(const unsigned char *ccnb, const struct ccn_indexbuf *comps,
							  size_t num_comps, size_t start, size_t stop,
							  struct ccn_pkey* key);

int replace_name(struct ccn_charbuf* dest, unsigned char* src,  size_t src_size, struct ccn_charbuf* name);

#endif /* SIGNED_INTEREST_H_ */
