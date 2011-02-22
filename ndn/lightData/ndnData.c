/*
 * ndnData.c
 *
 *  Created on: Feb 8, 2011
 *      Author: nesl
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

#include <ccn/ccn.h>
#include <ccn/uri.h>
#include <ccn/signing.h>

//#define URI "/ucla.edu/building/boelter/floor/3/room/3551/supply/1/fixture/1/R/250/G/000/B/000"
#define URI "/ucla.edu/building/boelter/floor/3/room/3551/supply/1/"
#define NUM_LIGHTS '4'
//#define URI "ccnx:/blah/blah"
//#define NAMEPREFIX "UCLA/REMAP/"
//#define SK1 "magickey13893"

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

static enum ccn_upcall_res
incoming_interest(
        struct ccn_closure* selfp,
        enum ccn_upcall_kind kind,
        struct ccn_upcall_info* info)
{
		int udpClient(char *data);
        printf("Received request (kind: %d).\n", kind);

        char suffix[4098] = "";  // starts with NUM LIGHTS = 4;
        suffix[0] = NUM_LIGHTS;  //NUM_LIGHTS;
        int i;
//      if (ccn_name_comp_to_str(info->interest_ccnb, info->interest_comps, 10) == 1) {
        for (i=10; i< info->pi->prefix_comps; i=i+2) {
                char *str;
                str = ccn_name_comp_to_str(info->interest_ccnb, info->interest_comps, i);
                printf("*** before good, i = %d, str = %s\n", i, str);
                strcat(suffix, "|");
                //printf("*** in the mid, i = %d\n", i);
                strcat(suffix, str);
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
        res = udpClient(data);
        printf("************ res = %d\n", res);

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
//	memset(buf, 0, 128);
//	strcat(buf, NAMEPREFIX);
//	struct ccn_charbuf *namebuf = NULL;
	struct ccn_closure in_interest = {
		.p = &incoming_interest
	};
	// ccn_signing_params --> Parameters for creating signed content objects.
	// The recommended way to use this is to create a local variable:
	// struct ccn_signing_params myparams = CCN_SIGNING_PARAMS_INIT; and then fill in the desired fields.
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

	/* Set up a handler for interests */
	ccn_set_interest_filter(ccn, name, &in_interest); //this name should be prefix of any interest name

// implement a function that search the csv file and get name (--> put in "buf") according to serial number @ here

/*#if 1
	sprintf(buf, "Hello World!\n");
	length = name->length;

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
*/

	printf("Event loop ...\n");
	ccn_run(ccn, -1);
	printf("******** here... \n");

	ccn_destroy(&ccn);
	printf("******** after ccn_destroy... n");

	return 0;
}

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


	host = (struct hostent *) gethostbyname((char *)"131.179.141.205"); //this IP addr must be the machine that sends out the data;
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

}

