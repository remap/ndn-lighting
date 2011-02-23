/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>

#include <iostream>

#include "CCNListener.h"
#include "CCNRegMsg.h"
#include "CCNPeerMap.h"
using namespace std;
#define MYPORT 4950  // the port users will be connecting to

#define MAXBUFLEN 1000

void* periodicMapCheck(void* p)
{
	CCNPeerMap* lpPeerMap = (CCNPeerMap*)p;
	while(1)
	{
		lpPeerMap->checkTimeOut();	
		lpPeerMap->print();
		
		sleep(5);
	}
	
	return NULL;

}

Listener::Listener(int p_iPort /*= CCNRP_DEFAULT_PORT*/)
  : m_iPort(p_iPort)
{

}

Listener::~Listener()
{

}

void Listener::listen()
{
	int sockfd;
//    int numbytes;
//    char buf[MAXBUFLEN];

	struct sockaddr_in myaddr;
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = INADDR_ANY;
	myaddr.sin_port = htons(m_iPort);
	
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (bind(sockfd, (struct sockaddr*)&myaddr, sizeof(myaddr)) == -1) {
        close(sockfd);
        perror("listener: bind");
    }

    //cout<<"listener: waiting to recvfrom..."<<endl;

	struct sockaddr_in their_addr;
	size_t addr_len;
    addr_len = sizeof their_addr;

	CCNPeerMap peerMap;
	
	pthread_t pc_thread;
	pthread_create(&pc_thread, NULL, periodicMapCheck, &peerMap);	
	
  bool bOK = true;
	while(bOK)
	{
    /*
		if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
			(struct sockaddr *)&their_addr, (socklen_t *)&addr_len)) == -1) {
			perror("recvfrom");
			exit(1);
		}

		// register the received messsage
		cout<<"listener: packet is "<<numbytes<<" bytes long"<<endl;
		buf[numbytes] = '\0';
		printf("listener: packet contains %s\n", buf);
		cout<<"listener: packet contains "<<buf<<endl;
		CCNRegMsg regMsg(T_REGISTRATION);
		regMsg.extractMsg(buf);
		regMsg.registerMsg(peerMap);
    */

    struct sockaddr_in tAddr;
    memset(&tAddr, 0, sizeof(tAddr));

		CCNRegMsg regMsg(T_REGISTRATION);
		if (!regMsg.extractMsg(sockfd, tAddr))
    {
      fprintf(stderr, "Unable to read message, aborting...\n");
      bOK = false;
    }
    else
    {
      // fprintf(stdout, "Got connection from %s\n", inet_ntoa(tAddr.sin_addr));

		  regMsg.registerMsg(peerMap);

		  // produce the peer list reply message
		  vector<CCNRegEntry> peerList = peerMap.getMap(regMsg);
	
		  CCNRegMsg peerListReply(T_PEERLIST_REPLY);

		  vector<CCNRegEntry>::iterator it;
		  for(it = peerList.begin(); it != peerList.end(); it++)
		  {
			  peerListReply.addEntry(*it);
		  }

      /*
		  //cout<<"PeerListReply message length:"<<peerListReply.m_uMsgLen<<endl;
		  char* replyMsg;
		  peerListReply.getMsg(&replyMsg);
		
		  if((numbytes = sendto(sockfd, replyMsg, peerListReply.getLength(), 0,
			  (struct sockaddr *)&their_addr, addr_len)) == -1) {
			  perror("sendto failed");
		  }
		  delete replyMsg;
      */

      if (!peerListReply.sendMsg(sockfd, tAddr))
      {
        fprintf(stderr, "Unable to send reply message\n");
      }
    }
	}
}
