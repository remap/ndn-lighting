#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include <fstream>
#include <string>
#include <iostream>
#include <sstream>

#include "dns_resolver.h"
#include "dns_a.h"
#include "dns_srv.h"
#include "dns_packet.h"

#include "RPClient.h"
#include "CCNRegMsg.h"
#include "Daemon.h"
#include "ccn_global.h"
#include "ccnrp_config.h"

using namespace std;

#define SERVER_PORT 9099
#define REFRESH_INTERVAL 120

void _usage()
{
  fprintf(stdout, "ccnrp_client <config file> [ -d ] | -h\n");
}

int _daemonize()
{
  int iRet = -1;
  pid_t tPid = -1;

  //fprintf(stderr, "Daemonizing...\n");
  if ((tPid = fork()) < 0)
  {
    fprintf(stderr, "Unable to fork: %s\n", strerror(errno));
  }
  else if (tPid > 0)
  {
    iRet = tPid;
  }
  else if (setsid() < 0)
  {
    fprintf(stderr, "Unable to create new process group: %s\n", strerror(errno));
  }
  else
  {
    close(0);
    close(1);
    close(2);
    int iFD = open("/dev/null", O_RDWR);
    if (iFD < 0)
    {
      fprintf(stderr, "Unable to open /dev/null: %s\n", strerror(errno));
    }
    else
    {
      int iRetVal = dup(iFD);
      iRetVal = dup(iFD);
      iRetVal = 0;

      if (chdir("/") < 0)
      {
        fprintf(stderr, "Unable to chdir to /: %s\n", strerror(errno));
      }
      else
      {
        iRet = 0;
      }
    }
  }

  return iRet;
}

bool _getRpAddr(std::string &p_sSrv, struct sockaddr_in &p_tAddr)
{
  bool bRet = false;

  DnsResolver oRes;
  oRes.init();
  DnsPacket oResp;
  RRList_t tList;
  if (!oRes.send(p_sSrv, DNS_RR_SRV, oResp))
  {
    fprintf(stderr, "Unable to get SRV for '%s'\n", p_sSrv.c_str());
  }
  else if (!oResp.getAnswers(tList))
  {
    fprintf(stderr, "Unable to get answers from SRV response.\n");
  }
  else
  {
    uint16_t uPort = 0;
    string sTarget;

    RRIter_t oIter;
    for (oIter = tList.begin();
         tList.end() != oIter;
         oIter++)
    {
      DnsRR *pRR = *oIter;
      if (DNS_RR_SRV == pRR->type())
      {
        DnsSrv *pSRV = static_cast<DnsSrv *>(pRR);
        uPort = pSRV->getPort();
        sTarget = pSRV->getTarget();

        // oResp.print();
        break;
      }
    }

    tList.clear();
    oResp.clear();
    if (0 != uPort)
    {
      if (!oRes.send(sTarget, DNS_RR_A, oResp))
      {
        fprintf(stderr, "Unable to send A record for '%s'\n", sTarget.c_str());
      }
      else if (!oResp.getAnswers(tList))
      {
        fprintf(stderr, "Unable to get answers from A response.\n");
      }
      else
      {
        uint32_t uIP = 0;
        for (oIter = tList.begin();
             tList.end() != oIter;
             oIter++)
        {
          DnsRR *pRR = *oIter;
          if (DNS_RR_A == pRR->type())
          {
            uIP = (static_cast<DnsA *>(pRR))->ip();
            break;
          }
        }

        if (0 != uIP)
        {
          p_tAddr.sin_addr.s_addr = uIP;
          p_tAddr.sin_port = uPort;
          bRet = true;
        }
      }
    }
  }

  return bRet;
}

int main(int argc, const char** argv)
{
  int iRet = 0;

  CcnrpConfig oConf;
  if (argc < 2)
  {
    fprintf(stderr, "Must specify config file\n");
    _usage();
    iRet = 1;
  }
  else if (0 == strcmp("-h", argv[0]))
  {
    _usage();
  }
  else if (!oConf.load(argv[1]))
  {
    fprintf(stderr, "Unable to laod file: %s\n", argv[1]);
    iRet = 1;
  }
  else
  {
    bool bDaemon = false;
    if (argc > 2)
    {
      if (0 == strncmp("-d", argv[2], 2))
      {
        bDaemon = true;
      }
    }

    const char *szRouteFile = oConf.getValue("ccn_route_file");
    if (NULL == szRouteFile)
    {
      fprintf(stderr, "Unable to find route file.\n");
    }
    else
    {
	    CCNRegEntry regEntry;

	    ifstream inSelfConf;
	    inSelfConf.open(szRouteFile);
	
	    if(!inSelfConf)
	    {
		    cerr<< "Unable to open file self.conf"<<endl;
		    exit(1);
	    }
		
	    uint16_t port;
	    uint16_t ttl;
	    string protocol;
	    string strNameSpace; 

		cout<<"Start client"<<endl;

	    string line;
	    CCNRegMsg regMsg(T_REGISTRATION);
	    while(getline(inSelfConf, line))
	    {
		    if(line[0] == '#') // this is a comment line
		    {
			    continue;
		    }
		
		    istringstream issLine;
		    issLine.str(line);
		    issLine>>port>>ttl>>protocol>>strNameSpace;
		
		    CCNRegEntry regEntry;
        //struct in_addr tAddr;
		    //inet_pton(AF_INET, strIP.c_str(), &tAddr);
        //regEntry.setIP(ntohl(tAddr.s_addr));
		    regEntry.setPort(port);
		    regEntry.setTTL(ttl);
		    regEntry.setProto(proto2n(protocol));
		    regEntry.setNameSpace(strNameSpace.c_str(), strNameSpace.length());

		    regMsg.addEntry(regEntry);
        //fprintf(stdout, "Entry from config file: %s\n", regEntry.toString().c_str());
	    }
      //fprintf(stdout, "---------\nJust made MESSAGE :\n");
      //regMsg.print();

	    inSelfConf.close();
	
      string sSRV;
      const char *szSrv = oConf.getValue("ccnrp_domain");
      if (NULL == szSrv)
      {
        sSRV = "_ccnrp._udp.secspider.cs.ucla.edu.";
      }
      else
      {
        sSRV = szSrv;
      }

      bool bRun = true;
      if (bDaemon)
      {
        bRun = false;
        int iPID = _daemonize();
        if (0 > iPID)
        {
          fprintf(stderr, "Unable to daemonize.  Exiting...\n");
          iRet = 1;
        }
        else if (0 < iPID)
        {
          fprintf(stdout, "ccnrp started w/ PID %d\n", iPID);
        }
        else
        {
          bRun = true;
        }
      }

      if (bRun)
      {
        iRet = 0;

	      do
	      {
          sockaddr_in tAddr;
          memset(&tAddr, 0, sizeof(tAddr));

          if (!_getRpAddr(sSRV, tAddr))
          {
            fprintf(stderr, "Unable to get RP SRV.\n");
          }
          else
          {
            tAddr.sin_addr.s_addr = htonl(tAddr.sin_addr.s_addr);
            char *szIP = inet_ntoa(tAddr.sin_addr);
            fprintf(stdout, "Connecting to server at %s : %u  (from %s SRV)\n",
            szIP,
            tAddr.sin_port,
            szSrv);
	
	          RPClient client(szIP, tAddr.sin_port);
//      RPClient client("192.168.0.115", SERVER_PORT);	
            char pBuff[CCNRP_MAX_PACKET_SIZE];
            memset(pBuff, 0, CCNRP_MAX_PACKET_SIZE);
            if (!regMsg.toWire(pBuff, CCNRP_MAX_PACKET_SIZE))
            {
              fprintf(stderr, "Unable to convert regMsg toWire()\n");
            }
            else
            {
              client.sendMsg(pBuff, regMsg.getLength());
				fprintf(stdout, "Registration message has been sent out\n");
            }
			client.recvMsg();
		
		        sleep(REFRESH_INTERVAL/2);
          }
	      }while(1);
      }
    }
  }

  return iRet;

}
