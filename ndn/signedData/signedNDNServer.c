#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

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
	//struct ccn** ccn_rec;
} handler_data;

static char *ccn_name_comp_to_str(
                                  const unsigned char *ccnb,
                                  const struct ccn_indexbuf *comps,
                                  int index)
{
    size_t comp_size;
    const unsigned char *comp_ptr;
    char *str;
    if (ccn_name_comp_get(ccnb, comps, index, &comp_ptr, &comp_size) == 0) {
        str = (char *)malloc(sizeof(char)*(comp_size + 1));
        strncpy(str, (const char *)comp_ptr, comp_size);
        str[comp_size] = '\0';
        return str;
        printf("str is got!"); // "str=%s\n", *str);
    }
    else {
        printf(stderr, "Cannot get comp with index %d\n", index);
        return NULL;
    }
    
}

// Our test content URI; the code appends a nonce.
//const char* TEST_URI = "ccnx:/data_for_a_signed_interest";
const char* TEST_URI = "/ucla.edu/apps/lighting/";
#define NUM_LIGHTS '4'

int main(int argc, char** argv) {
	int res = 0;
	struct ccn* ccn_pub;
	//struct ccn* ccn_rec;
    
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
    /*
     ccn_rec = ccn_create();
     if (ccn_connect(ccn_rec, NULL) == -1) {
     fprintf(stderr, "Could not connect to ccnd");
     return(1);
     }
     */
    // Closure to handle upcalls
    struct ccn_closure *cl = NULL;
    cl = calloc(1, sizeof(*cl));
    cl->p = &packet_handler;
    handler_data h_data = { &complete, &outstanding_interests, &public_key, &ccn_pub};
    cl->data = &h_data;
    
    // Setup our one test name without signature
    struct ccn_charbuf* name;
    name = ccn_charbuf_create();
    ccn_name_from_uri(name, TEST_URI);
    //ccn_name_append_nonce(name);
    //fprintf(stderr, "Our name: %s/<nonce>\n", TEST_URI);
    
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
	// Construct a key locator containing the key itself 
    build_keylocator_from_key(&keylocator, public_key);
    /*
     // And a SignedInfo too
     struct ccn_charbuf *signed_info = ccn_charbuf_create();
     res = ccn_signed_info_create(signed_info,
     public_key_digest,
     public_key_digest_length,
     timestamp,
     p.type,
     p.freshness,
     0,  // FinalBlockID is optional
     keylocator);
     */
	/*
     // Sign the interest
     struct ccn_charbuf *name_signed = ccn_charbuf_create();
     sign_interest(name_signed, name, signed_info, NULL, private_key);
     
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
     ccn_run(ccn_rec, 100); // stop if we run dry for .1 sec 
     ccn_run(ccn_pub, 100); // stop if we run dry for .1 sec 
     fflush(stdout);
     }
     */
	fprintf(stderr, "listening...");
	ccn_run(ccn_pub, -1);
    
    ccn_charbuf_destroy(&timestamp);
    ccn_charbuf_destroy(&keylocator);
    ccn_charbuf_destroy(&finalblockid);
    //    ccn_charbuf_destroy(&signed_info);
    ccn_charbuf_destroy(&name);
    
    // ccn_charbuf_destroy(&name_signed);
    // ccn_charbuf_destroy(&name_signed_copy);
    ccn_destroy(&ccn_pub);
    //ccn_destroy(&ccn_rec);
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
            fprintf(stderr, "CCN_UPCALL_INTEREST, (matched comp  == %d)\n", info->matched_comps);
            fprintf(stderr, "                     (interest comps == %zu)\n", info->interest_comps->n);
            int res = 0;
            // Corrected 20-May-2011 to support interests with additional components after prefix
            //
            if (info->interest_comps->n < 3) {	// Name + signature + implicit digest, minimum
                fprintf(stderr, "\tnot enough components, %zu<3\n", info->interest_comps->n);
            } else {
                // Verify the interest
                res = verify_signed_interest(info->interest_ccnb, info->interest_comps,
                                             info->interest_comps->n-2, info->interest_comps->buf[0], info->interest_comps->buf[info->interest_comps->n-2],
                                             (*h_data->public_key));
                fprintf(stderr, "\tverify_signed_interest == %d (%s)\n", res, (res==1)?"verified":"unverified");
            }
            
            if (res==1){ 
                processInterestForLighting(upcall_kind, info);
            } else {
                processInterestForLighting(upcall_kind, info); 
                fprintf(stderr,"Unverified interest, not processing for lighting...");
            }
            
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

// udpClient for ColorBlast lights
int udpClient(char *send_data) {
    
	printf("********** in udpClient\n");
	int sock;
	struct sockaddr_in server_addr;
	struct hostent *host;
	int count = 1;
	//char *send_data = data;
	char *prev_data;
    
	printf("********** after assign the send_data\n");
	printf("********** send_data = %s\n", send_data);
    
    
	host = (struct hostent *) gethostbyname((char *)"127.0.0.1"); //this IP addr must be the machine that sends out the data;
    //	host = (struct hostent *) gethostbyname((char *)"172.17.5.222"); //this IP addr must be the machine that sends out the data;
    
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}
    
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(50009);
	server_addr.sin_addr = *((struct in_addr *)host->h_addr);
	bzero(&(server_addr.sin_zero),8);
    
    //	while (1) {
    
    //printf("Type something (q or Q to quit)");
    //gets(send_data);
    
    // NUM LIGHTS | ID1 | R | G | B | ID2 | R | ...
    //		send_data = "4|1|250|086|100";
    //		if ((strcmp(send_data, "q") == 0) || strcmp(send_data, "Q") == 0)
    //			break;
    //		else
    
	if (count == 1) {
		sendto(sock, send_data, strlen(send_data), 0, (struct sockaddr *)&server_addr, sizeof(struct sockaddr));
		printf("********* sent... \n");
        
		count++;
		char *prev_data = send_data;
	}
	else if (prev_data == send_data) {
		sendto(sock, send_data, strlen(send_data), 0, (struct sockaddr *)&server_addr, sizeof(struct sockaddr));
		printf("********* sent... \n");
		printf("******** count %d\n", count);
        
		count++;
		char *prev_data = send_data;
	}
    close(sock);
    
}

// udpClient2 for ArtNet lights
int udpClient2(char *send_data) {
	
	printf("********** in udpClient2\n");
	int sock;
	struct sockaddr_in server_addr;
	struct hostent *host;
	int count = 1;
	char *prev_data;
	
	printf("********** after assign the send_data\n");
	printf("********** send_data = %s\n", send_data);
	
	
	host = (struct hostent *) gethostbyname((char *)"127.0.0.1"); //this IP addr must be the machine that sends out the data;
	
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}
	
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(50010);
	server_addr.sin_addr = *((struct in_addr *)host->h_addr);
	bzero(&(server_addr.sin_zero),8);
	
	if (count == 1) {
		sendto(sock, send_data, strlen(send_data), 0, (struct sockaddr *)&server_addr, sizeof(struct sockaddr));
		printf("********* sent... \n");
		
		count++;
		char *prev_data = send_data;
	}
	else if (prev_data == send_data) {
		sendto(sock, send_data, strlen(send_data), 0, (struct sockaddr *)&server_addr, sizeof(struct sockaddr));
		printf("********* sent... \n");
		printf("******** count %d\n", count);
		
		count++;
		char *prev_data = send_data;
	}
    close(sock);
	
}

// udpClient3 for iColorFlex string lights
int udpClient3(char *send_data) {
	
	printf("********** in udpClient3\n");
	int sock;
	struct sockaddr_in server_addr;
	struct hostent *host;
	int count = 1;
	char *prev_data;
	
	printf("********** after assign the send_data\n");
	printf("********** send_data = %s\n", send_data);
	
	
	host = (struct hostent *) gethostbyname((char *)"127.0.0.1"); //this IP addr must be the machine that sends out the data;
	
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}
	
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(50011);
	server_addr.sin_addr = *((struct in_addr *)host->h_addr);
	bzero(&(server_addr.sin_zero),8);
	
	if (count == 1) {
		sendto(sock, send_data, strlen(send_data), 0, (struct sockaddr *)&server_addr, sizeof(struct sockaddr));
		printf("********* sent... \n");
		
		count++;
		char *prev_data = send_data;
	}
	else if (prev_data == send_data) {
		sendto(sock, send_data, strlen(send_data), 0, (struct sockaddr *)&server_addr, sizeof(struct sockaddr));
		printf("********* sent... \n");
		printf("******** count %d\n", count);
		
		count++;
		char *prev_data = send_data;
	}
    close(sock);
	
}



int processInterestForLighting(enum ccn_upcall_kind kind, struct ccn_upcall_info* info){
    
    int udpClient(char *data);
    printf("Received request (kind: %d).\n", kind);
    
    char suffix[4098] = "";  // starts with NUM LIGHTS = 4;
    char *fixType;
    //suffix[0] = NUM_LIGHTS;  //NUM_LIGHTS; **
    int i;
    //      if (ccn_name_comp_to_str(info->interest_ccnb, info->interest_comps, 10) == 1) {
    // URI start from index 0
    for (i=0; i< info->pi->prefix_comps; i++) {
        char *str;
        //char *fix = "fixture";
        //char *rgb8 = "rgb-8bit-hex";
        str = ccn_name_comp_to_str(info->interest_ccnb, info->interest_comps, i);
        printf("*** before good, i = %d, str = %s\n", i, str);
        //if (strcmp(str, fix) == 0) {
        if (strcmp(str, "fixture") == 0) {
            printf("****** in if fixture\n ");
			
			//add by Chenni on May 19
			char *temp;
			fixType = ccn_name_comp_to_str(info->interest_ccnb, info-> interest_comps, i+1);
			
			if (strcmp(fixType, "ColorBlast") == 0) {
				suffix[0] = NUM_LIGHTS;  //NUM_LIGHTS; **
				strcat(suffix, "|");
				temp = ccn_name_comp_to_str(info->interest_ccnb, info->interest_comps, i+2);
				strcat(suffix, temp);
			}
			else if (strcmp(fixType, "ArtNet") == 0) {
				printf("*** ArtNet controller ***\n");
				char *ch;
				char *d;
				ch = ccn_name_comp_to_str(info->interest_ccnb, info->interest_comps, i+2);
				d = ccn_name_comp_to_str(info->interest_ccnb, info->interest_comps, i+3);
				strcat(suffix, ch);
				strcat(suffix, "|");
				strcat(suffix, d);
			}
			else if (strcmp(fixType, "iColorFlex") == 0) {
				printf("*** iColorFlex lighting ***\n");
				char *istring;
				char *ich;
				//char *ir;
				//char *ig;
				//char *ib;
				istring = ccn_name_comp_to_str(info->interest_ccnb, info->interest_comps, i+2);
				ich = ccn_name_comp_to_str(info->interest_ccnb, info->interest_comps, i+3);
				strcat(suffix, istring);
				strcat(suffix, "|");
				strcat(suffix, ich);
			}
			
        }
        
        //if (strcmp(str, rgb8) == 0) {
        if (strcmp(str, "rgb-8bit-hex") == 0) {
            
            printf("****** in if rgb-8bit-hex\n");
            
            char *rgb;
            char *r_hex = (char *) malloc(2), *g_hex = (char *) malloc(2), *b_hex = (char *) malloc(2);
            char *r_str = (char *) malloc(3), *g_str = (char *) malloc(3), *b_str = (char *) malloc(3);
            int r_dec, g_dec, b_dec;
            
            // Get interest command RGB
            rgb = ccn_name_comp_to_str(info->interest_ccnb, info->interest_comps, i+1);
            printf("****** rgb = %s\n", rgb);
            
            // Parse RGB to R, G, B
            r_hex = strncpy(r_hex, rgb, 2);
            g_hex = strncpy(g_hex, rgb+2, 2);
            b_hex = strncpy(b_hex, rgb+4, 2);
            printf("***** r = %s, g = %s, b= %s\n", r_hex, g_hex, b_hex);
            
            // Convert RGB in hex to decimal
            r_dec = strtol (r_hex, (char **) NULL, 16);
            g_dec = strtol (g_hex, (char **) NULL, 16);
            b_dec = strtol (b_hex, (char **) NULL, 16);
            printf("****** r = %d, g = %d, b = %d\n", r_dec, g_dec, b_dec);
            
            // Convert RGB in int to string
            sprintf(r_str, "%d", r_dec);
            sprintf(g_str, "%d", g_dec);
            sprintf(b_str, "%d", b_dec);
            printf("***** r = %s, g = %s, b = %s \n", r_str, g_str, b_str);
            
            // Construct suffix and get ready to be sent out over UDP
            strcat(suffix, "|");
            strcat(suffix, r_str);
            strcat(suffix, "|");
            strcat(suffix, g_str);
            strcat(suffix, "|");
            strcat(suffix, b_str);
            printf("***** suffix = %s\n", suffix);
            
            free(r_hex);
            free(g_hex);
            free(b_hex);
            free(r_str);
            free(g_str);
            free(b_str);
            
        }
        //strcat(suffix, "|");
        //printf("*** in the mid, i = %d\n", i);
        //strcat(suffix, str);
        printf("*** good here, i = %d\n", i);
        free(str);
    }
    //      }
    printf("suffix is %s\n", suffix);
    if (info->pi->prefix_comps+1 > info->interest_comps->n) {
        printf("how did this happen? comps->n is %d and prefix_comps is %d\n", (int)info->pi->prefix_comps);
    }
    printf("*********** here...\n");
    
    char *data = suffix;
    int res;
	
	//add
	if (strcmp(fixType, "ColorBlast") == 0)
		res = udpClient(data);
	else if (strcmp(fixType, "ArtNet") == 0)
		res = udpClient2(data);
	else if (strcmp(fixType, "iColorFlex") == 0)
		res = udpClient3(data);
	
    printf("************ res = %d\n", res);
    
    
}
