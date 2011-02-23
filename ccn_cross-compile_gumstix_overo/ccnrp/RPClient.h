#ifndef _RP_CLIENT_H
#define _RP_CLIENT_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>

#include "CCNRegMsg.h"

class RPClient
{
private:
	struct sockaddr_in si_other;
	int sockfd;	
  CCNRegMsg m_oCurrentTable;

public:
	RPClient(char* rpServer, int port);
	virtual ~RPClient();

	void sendMsg(void* msg, int length);
	void recvMsg();
};
#endif
