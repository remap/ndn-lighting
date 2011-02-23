#include "RPClient.h"
#include "CCNRegMsg.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include "ccn_global.h"
using namespace std;

#define MAX_RETRANSMIT 10

RPClient::RPClient(char* rpServer, int port)
	: m_oCurrentTable(T_PEERLIST_REPLY)
{
	if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		perror("cannot create socket");
		exit(1);
	}

	memset((char*) &si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(port);
	if(inet_pton(AF_INET, rpServer, &si_other.sin_addr) == -1)
	{
		cout<<"inet_aton failed"<<endl;
		exit(1);
	}
}

void RPClient::sendMsg(void* msg, int length)
{
	fd_set fds;
	struct timeval timeout;
	timeout.tv_sec = 3;
	timeout.tv_usec = 0;
	FD_ZERO(&fds);

    int numbytes;

	int retransmit = 0;
	do
	{
    	if ((numbytes = sendto(sockfd, msg, length, 0,
             (struct sockaddr*)&si_other, sizeof si_other)) == -1) {
        	perror("talker: sendto");
        	exit(1);
    	}
		//cout<<"send out message"<<endl;

		FD_SET(sockfd, &fds);
		select(sockfd + 1, &fds, NULL, NULL, &timeout);
		retransmit++;
		if(retransmit >= MAX_RETRANSMIT)
			break;
	}while(!FD_ISSET(sockfd, &fds));

	if(retransmit >= MAX_RETRANSMIT)
	{
		cout<<"retransmitted "<<MAX_RETRANSMIT<<" times without success"<<endl;
		exit(1);
	}
	
	//recvMsg();
}

void RPClient::recvMsg()
{
	m_oCurrentTable.extractMsg(sockfd, si_other);
	if(m_oCurrentTable.getEntryNum() != 0)
	{
		fprintf(stdout, "CCN peer(s) found. Adding them to ccndcontrol:\n");
		map<string, CCNRegEntry>::iterator it = m_oCurrentTable.begin();
		for(; it != m_oCurrentTable.end(); it++)
		{
			string strControl = "ccndcontrol add " + (it->second).toString();
			cout<<strControl<<endl;
	 		int iSysRet = system(strControl.c_str());
			if (0 != iSysRet)
			{
				fprintf(stderr, "Error %d executing ccndcontrol.\n", iSysRet);
			}
		}
		//m_oCurrentTable.print();	
	}
	else{
		cout<<"No peer available"<<endl;
	}
}
	

RPClient::~RPClient()
{
  close(sockfd);
}
