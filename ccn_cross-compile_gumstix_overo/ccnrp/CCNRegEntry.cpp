#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <sstream>
#include <iostream>
#include <sys/socket.h>

#include "CCNRegEntry.h" 
#include "ccnrp_defs.h"
#include "ccn_global.h" 

using namespace std;

CCNRegEntry::CCNRegEntry()
  : m_pMsg(NULL),
    m_pBuffer(NULL),
    m_uLen(0)
{
  setNameSpace(NULL, 0);
	setIP(0);	
}

int CCNRegEntry::extractEntry(char *pEntry)
{
  int iRet = 0;

  ccnrp_peer_msg_t *pMsg = (ccnrp_peer_msg_t *) pEntry;
  if (NULL == pMsg)
  {
    fprintf(stderr, "Unable to parse NULL message.\n");
  }
  else
  {
    setNameSpace(pMsg->m_pNS, (int) pMsg->m_uNsLen);
    memcpy(m_pMsg, pMsg, sizeof(ccnrp_peer_msg_t));
    iRet = sizeof(ccnrp_peer_msg_t) + (int) pMsg->m_uNsLen;
//    iRet = (int) m_uLen;

  }

  return iRet;
}

int CCNRegEntry::toWire(char *p_pBuff, size_t p_uLen)
{
  int iRet = -1;

  size_t uNeededLen = sizeof(ccnrp_peer_msg_t) + getNsLen();
  if (NULL == p_pBuff)
  {
    fprintf(stderr, "Unable to serialize entry toWire() with NULL input buffer.\n");
  }
  else if (p_uLen < uNeededLen)
  {
    fprintf(stderr, "Unable to serialize entry toWire() because buffer is too small: %u < %u\n",
            (unsigned) p_uLen,
            (unsigned) uNeededLen);
  }
  else
  {
    memcpy(p_pBuff, m_pBuffer, uNeededLen);
    iRet = (int) uNeededLen;
  }

  return iRet;
}

/*
int CCNRegEntry::extractEntry(char** pEntry)
{
	uint32_t ip;
	uint16_t port;
	uint16_t ttl;
	mempop(&ip, pEntry, sizeof ip);
	m_uIP = ntohl(ip);
	mempop(&port, pEntry, sizeof port);
	m_uPort = ntohs(port);
	mempop(&ttl, pEntry, sizeof ttl);
	m_uTTL = ntohs(ttl);
	mempop(&m_uProtocol, pEntry, sizeof m_uProtocol);
	mempop(&m_uNameLen, pEntry, sizeof m_uNameLen);

	m_ccn_namespace = new char[m_uNameLen + 1];
	mempop(m_ccn_namespace, pEntry, m_uNameLen);
	m_ccn_namespace[m_uNameLen] = '\0';

	return 10 + m_uNameLen;
}
*/

string CCNRegEntry::toString() const
{
	stringstream ss;

	char strIP[INET_ADDRSTRLEN];
	uint32_t ip = ntohl(getIP());
	inet_ntop(AF_INET, &ip, strIP, INET_ADDRSTRLEN);

	ss<<"ccnx:"<<m_pMsg->m_pNS<<" "<<n2proto(getProto())<<" "<<strIP<<" "<<getPort();
	return ss.str();
}

string CCNRegEntry::getKey() const
{
	stringstream ssKey;
	ssKey<<m_pMsg->m_uIP<<","<<m_pMsg->m_uPort<<","<<m_pMsg->m_uProto<<","<<m_pMsg->m_pNS;	

	return ssKey.str();
}

bool CCNRegEntry::operator==(const CCNRegEntry& regEntry)
{
  /*
	return m_uIP == regEntry.m_uIP && m_uPort == regEntry.m_uPort &&
		m_uTTL == regEntry.m_uTTL && m_uProtocol == regEntry.m_uProtocol &&
		m_uNameLen == regEntry.m_uNameLen && 
		(strncmp(m_ccn_namespace, regEntry.m_ccn_namespace, m_uNameLen) == 0);
  */
  return m_uLen == regEntry.m_uLen && !memcmp(m_pMsg, regEntry.m_pMsg, m_uLen);
}

CCNRegEntry& CCNRegEntry::operator=(const CCNRegEntry& regEntry)
{
	setIP(regEntry.getIP());
	setPort(regEntry.getPort());
	setTTL(regEntry.getTTL());
	setProto(regEntry.getProto());
  size_t uLen = 0;
  const char *szNS = regEntry.getNameSpace();
  if (NULL != szNS)
  {
    uLen = strlen(szNS);
  }
  setNameSpace(szNS, uLen);
	return *this;
}
CCNRegEntry::CCNRegEntry(const CCNRegEntry& regEntry)
  : m_pMsg(NULL),
    m_pBuffer(NULL),
    m_uLen(0)
{
  setNameSpace(NULL, 0);
	*this = regEntry;
}

CCNRegEntry::~CCNRegEntry()
{
  if (NULL != m_pBuffer)
  {
    delete[] m_pBuffer;
    m_pBuffer = NULL;
    m_uLen = 0;
  }
}

size_t CCNRegEntry::getLen()
{
  return m_uLen;
}

uint32_t CCNRegEntry::getIP() const
{
  return ntohl(m_pMsg->m_uIP);
}

void CCNRegEntry::setIP(uint32_t p_uIP)
{
  m_pMsg->m_uIP = htonl(p_uIP);
}


uint16_t CCNRegEntry::getPort() const
{
  return ntohs(m_pMsg->m_uPort);
}

void CCNRegEntry::setPort(uint16_t p_uPort)
{
  m_pMsg->m_uPort = htons(p_uPort);
}


uint16_t CCNRegEntry::getTTL() const
{
  return ntohs(m_pMsg->m_uTTL);
}

void CCNRegEntry::setTTL(uint16_t p_uTTL)
{
  m_pMsg->m_uTTL = htons(p_uTTL);
}


uint8_t CCNRegEntry::getProto() const
{
  return m_pMsg->m_uProto;
}

void CCNRegEntry::setProto(uint8_t p_uProto)
{
  m_pMsg->m_uProto = p_uProto;
}


const char *CCNRegEntry::getNameSpace() const
{
  return (const char *) m_pMsg->m_pNS;
}

time_t CCNRegEntry::getLastSeen() const
{
  return m_lastSeen;
}

void CCNRegEntry::setLastSeen(time_t p_tSeen)
{
  m_lastSeen = p_tSeen;
}

void CCNRegEntry::setNameSpace(const char* ccnNameSpace, int length)
{
  m_uLen = (size_t) length + sizeof(ccnrp_peer_msg_t);
  char *pBuff = new char[m_uLen + 1];
  memset(pBuff, 0, m_uLen + 1);

  if (NULL != m_pBuffer)
  {
    memcpy(pBuff, m_pBuffer, sizeof(ccnrp_peer_msg_t));
    delete[] m_pBuffer;
  }
  m_pBuffer = pBuff;
  m_pMsg = (ccnrp_peer_msg_t *) m_pBuffer;

  m_pMsg->m_uNsLen = (uint8_t) length;
  if (length > 0)
  {
	  memcpy((char *) m_pMsg->m_pNS, (char *) ccnNameSpace, length);
  }
}

uint8_t CCNRegEntry::getNsLen()
{
  return m_pMsg->m_uNsLen;
}

uint16_t CCNRegEntry::getEntryLen() const
{
/*
	return (sizeof m_uIP) + (sizeof m_uPort) + (sizeof m_uTTL) +
		(sizeof m_uProtocol) + (sizeof m_uNameLen) + m_uNameLen;
*/
  return (uint16_t) m_uLen;
}

ccnrp_peer_msg_t *CCNRegEntry::getEntry()
{

  /*
	msg = new char[getEntryLen()];

	char* p_unset = msg;

	uint16_t port = htons(m_uPort);
	uint16_t ttl = htons(m_uTTL);

	mempush(&p_unset, &m_uIP, sizeof m_uIP);
	mempush(&p_unset, &port, sizeof port);
	mempush(&p_unset, &ttl, sizeof ttl);
	mempush(&p_unset, &m_uProtocol, sizeof m_uProtocol);
	mempush(&p_unset, &m_uNameLen, sizeof m_uNameLen);
	mempush(&p_unset, m_ccn_namespace, m_uNameLen);

	return (void*)msg;	
  */

  return m_pMsg;
}
