#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <ccn/ccn.h>
#include <ccn/uri.h>
#include <ccn/signing.h>

#define URI "light1"
#define NAMEPREFIX "UCLA/REMAP/"

#define UNUSED(expr) UNUSED_ ## expr __attribute__((unused))

static enum ccn_upcall_res
incoming_interest(
	struct ccn_closure* UNUSED(selfp),
	enum ccn_upcall_kind kind,
	struct ccn_upcall_info* UNUSED(info))
{
	printf("Received request (kind: %d).\n", kind);
	return CCN_UPCALL_RESULT_OK;
}

int main(void)
{
	int res;
	struct ccn *ccn = NULL;
	struct ccn_charbuf *name = NULL;
	struct ccn_charbuf *result;
	size_t length;
	char buf[128];
	memset(buf, 0, 128);
	strcat(buf, NAMEPREFIX);
	struct ccn_charbuf *namebuf = NULL;
	struct ccn_closure in_interest = {
		.p = &incoming_interest
	};
	struct ccn_signing_params sp = CCN_SIGNING_PARAMS_INIT;

	name = ccn_charbuf_create();

	res = ccn_name_from_uri(name, URI);
	if (res < 0) {
		fprintf(stderr, "bad ccn URI: %s\n", URI);
		exit(1);
	}
	ccn = ccn_create();

	res = ccn_connect(ccn, NULL);
	if (res < 0) {
		fprintf(stderr, "can't connect: %d\n", res);
		ccn_perror(ccn, "ccn_connect");
		exit(1);
	}

	ccn_set_interest_filter(ccn, name, &in_interest); 

#if 1
	namebuf = ccn_charbuf_create();
	namebuf->buf = strcat(buf, URI);
	printf("%s\n", namebuf->buf);
	length = namebuf->length;

	sp.sp_flags |= CCN_SP_FINAL_BLOCK;

	result = ccn_charbuf_create();
	res = ccn_sign_content(ccn, result, name, &sp, buf, length);
	if (res < 0) {
		ccn_perror(ccn, "ccn_sign_content");
		exit(1);
	}
	ccn_charbuf_destroy(&name);

	printf("Sending the data...\n");

	res = ccn_put(ccn, result->buf, result->length);
	if (res < 0) {
		ccn_perror(ccn, "ccn_put");
		exit(1);
	}
	ccn_charbuf_destroy(&result);
#endif

	printf("Event loop ...\n");
	ccn_run(ccn, -1);

	ccn_destroy(&ccn);

	return 0;
}


