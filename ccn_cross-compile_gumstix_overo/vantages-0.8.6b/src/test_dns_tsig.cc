#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>

#include <string>

#include "dns_defs.h"
#include "dns_resolver.h"
#include "dns_tsig.h"
#include "dns_packet.h"

using namespace std;

void _usage()
{
  fprintf(stdout, "test_dns_tsig <key file> <domain name> <name server IP> <port>\n");
}

int main(int argc, char *argv[])
{
  int iRet = 0;

  if (argc < 5)
  {
    fprintf(stderr, "Improper parameters.\n");
    _usage();
  }
  else
  {
    char *szFile = argv[1];
    string sDomain = argv[2];
    char *szIP = argv[3];
    int iPort = atoi(argv[4]);
    struct in_addr tAddr;

    if (!inet_aton(szIP, &tAddr))
    {
      fprintf(stderr, "Unable to convert '%s' to an IP: %s\n", szIP, strerror(errno));
    }
    else
    {
      DnsResolver oRes;
      DnsPacket oResp;
      oRes.setNameserver(ntohl(tAddr.s_addr));
      oRes.setPort(iPort);
      oRes.setBuffSize(4096);

      if (!oRes.setTsig(szFile))
      {
        fprintf(stderr, "Unable to load file.\n");
      }
      else if (!oRes.send(sDomain, oResp))
      {
        fprintf(stderr, "Unable to send.\n");
      }
      else
      {
        fprintf(stdout, "Got back:\n");
        oResp.print();
        fprintf(stdout, ">>>SUCCESS<<<\n");
      }
    }
  }

  return iRet;
}
