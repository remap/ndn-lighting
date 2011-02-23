#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <iostream>

#include "CCNRegMsg.h"
#include "ccn_global.h"
#include "ccnrp_defs.h"

using namespace std;

CCNRegMsg::CCNRegMsg(uint8_t _type, uint8_t _version)
{
	m_uVersion = _version;
	m_uMsgType = _type;
	m_uMsgLen = 4;
}

CCNRegMsg::CCNRegMsg(const CCNRegMsg &p_oRHS)
{
  *this = p_oRHS;
}

CCNRegMsg::~CCNRegMsg()
{
}

map<string, CCNRegEntry>::iterator CCNRegMsg::begin()
{
	return m_entryMap.begin();
}

map<string, CCNRegEntry>::iterator CCNRegMsg::end()
{
	return m_entryMap.end();
}

CCNRegMsg &CCNRegMsg::operator=(const CCNRegMsg &p_oRHS)
{
  m_uVersion = p_oRHS.m_uVersion;
  m_uMsgType = p_oRHS.m_uMsgType;
  m_uMsgLen = p_oRHS.m_uMsgLen;
  m_entryMap = p_oRHS.m_entryMap;

  return *this;
}

bool CCNRegMsg::extractMsg(int p_iSocket, struct sockaddr_in &p_tAddr)
{
  bool bRet = false;

  socklen_t tLen = sizeof(p_tAddr);
  ccnrp_header_t *pHeader = NULL;

  m_entryMap.clear();

  char pBuff[CCNRP_MAX_PACKET_SIZE];
  memset(pBuff, 0, CCNRP_MAX_PACKET_SIZE);
  int iCount = 0;
  if (-1 == (iCount = recvfrom(p_iSocket,
                               pBuff,
                               CCNRP_MAX_PACKET_SIZE,
                               0,
                               (struct sockaddr *)&p_tAddr,
                               &tLen)))
  {
    fprintf(stderr, "Unable to recvfrom(): %s\n", strerror(errno));
  }
  else
  {
    // fprintf(stdout, "Just got message from: %s\n", inet_ntoa(p_tAddr.sin_addr));

    pHeader = (ccnrp_header_t *) pBuff;
    int iLen = ntohs(pHeader->m_uLen);
    int iOffset = sizeof(ccnrp_header_t);

    if (iLen >= CCNRP_MAX_PACKET_SIZE)
    {
      fprintf(stderr, "Unable to accept packet of size %d (bigger than %u)\n", iLen, CCNRP_MAX_PACKET_SIZE);
    }
    else if (iCount != iLen)
    {
      fprintf(stderr, "Got the wrong nuber of bytes from recvfrom() %d != %d\n", iCount, iLen);
    }
    else
    {
      bRet = true;

      while (iOffset < iLen)
      {
        CCNRegEntry oEntry;
        int iTmp = oEntry.extractEntry(&(pBuff[iOffset]));
        if (iTmp <= 0)
        {
          fprintf(stderr, "Unable to read entry?\n");
          bRet = false;
          break;
        }
        else
        {
          iOffset += iTmp;
          if (T_REGISTRATION == m_uMsgType)
          {
            oEntry.setIP(ntohl(p_tAddr.sin_addr.s_addr));
          }
          m_entryMap[oEntry.getKey()] = oEntry;
        }
      }
    }
  }

  return bRet;
}

/*
void CCNRegMsg::extractMsg(char* msg)
{
	char* p = msg;
	mempop(&m_uVersion, &p, sizeof m_uVersion);
	mempop(&m_uMsgType, &p, sizeof m_uMsgType);
	uint16_t msgLen;
	mempop(&msgLen, &p, sizeof msgLen);
	m_uMsgLen = ntohs(msgLen);

	
	int consumedMsgLen = 4;
	
	while(consumedMsgLen < m_uMsgLen)
	{
		CCNRegEntry regEntry;
		consumedMsgLen += regEntry.extractEntry(&p);		
		m_entryMap[regEntry.getKey()] = regEntry;
	}
}
*/

void CCNRegMsg::print()
{
	map<string, CCNRegEntry>::iterator it;
	for(it = m_entryMap.begin(); it != m_entryMap.end(); it++)
	{
		cout<<(it->second).toString()<<endl;
	}
}

void CCNRegMsg::registerMsg(CCNPeerMap& peerMap)
{
	map<string, CCNRegEntry>::iterator it;
	for(it = m_entryMap.begin(); it != m_entryMap.end(); it++)
	{
		peerMap.addDiffEntry(it->second.getKey(), it->second);
	}
}

void CCNRegMsg::addEntry(const CCNRegEntry& _entry)
{
	m_entryMap[_entry.getKey()] = _entry;
	m_uMsgLen += _entry.getEntryLen();	
}

int CCNRegMsg::getLength()
{
	return m_uMsgLen;
}

int CCNRegMsg::getEntryNum()
{
	return m_entryMap.size();
}

void CCNRegMsg::getMsg(char** message)
{
	*message = new char[m_uMsgLen];
	//p_unsetMessage points to the memory that has been set to
	//the right value
	char* p_unset = *message; 
	mempush(&p_unset, &m_uVersion, sizeof m_uVersion);
	mempush(&p_unset, &m_uMsgType, sizeof m_uMsgType);
	
	uint16_t msgLength = htons(m_uMsgLen);
	mempush(&p_unset, &msgLength, sizeof msgLength);
	
	map<string,CCNRegEntry>::iterator it;
	for(it = m_entryMap.begin(); it != m_entryMap.end(); it++)
	{
		mempush(&p_unset, (it->second).getEntry(), (it->second).getEntryLen());
	}
}

bool CCNRegMsg::toWire(char *p_pBuff, size_t p_uLen)
{
  bool bRet = false;

  if (NULL == p_pBuff)
  {
    fprintf(stderr, "Input buffer is NULL\n");
  }
  else
  {
    size_t uLen = sizeof(ccnrp_header_t);
    map<string, CCNRegEntry>::iterator oIter;
    for (oIter = m_entryMap.begin();
         m_entryMap.end() != oIter;
         oIter++)
    {
      uLen += oIter->second.getLen();
    }

    if (uLen > CCNRP_MAX_PACKET_SIZE)
    {
      fprintf(stderr, "Unable to send packet of size %u because it is bigger than the max: %d\n",
              (unsigned) uLen,
              CCNRP_MAX_PACKET_SIZE);
    }
    else if (uLen > p_uLen)
    {
      fprintf(stderr, "Unable to send packet of size %u because it is bigger than the input buffer: %d\n",
              (unsigned) uLen,
              (unsigned) p_uLen);
    }
    else
    {
      m_uMsgLen = uLen;
      memset(p_pBuff, 0, p_uLen);

      int iOffset = sizeof(ccnrp_header_t);
      ccnrp_header_t *pHeader = (ccnrp_header_t *) p_pBuff;
      pHeader->m_uVersion = m_uVersion;
      pHeader->m_uType = m_uMsgType;
      pHeader->m_uLen = htons(uLen);

      bRet = true;
      for (oIter = m_entryMap.begin();
           m_entryMap.end() != oIter;
           oIter++)
      {
        int iTmp = oIter->second.toWire(&(p_pBuff[iOffset]), uLen - iOffset);
        if (iTmp <= 0)
        {
          fprintf(stderr, "Unable to extract entry\n");
          bRet = false;
          break;
        }
        else
        {
          iOffset += iTmp;
        }
      }
    }
  }

  return bRet;
}

bool CCNRegMsg::sendMsg(int p_iSocket, struct sockaddr_in &p_tAddr)
{
  bool bRet = false;

  char pBuff[CCNRP_MAX_PACKET_SIZE];
  memset(pBuff, 0, CCNRP_MAX_PACKET_SIZE);

  if (!toWire(pBuff, CCNRP_MAX_PACKET_SIZE))
  {
    fprintf(stderr, "Unable to send message because it cannot be converted toWire().\n");
  }
  else
  {
    socklen_t tLen = sizeof(p_tAddr);
    int iTmp = sendto(p_iSocket, pBuff, getLength(), 0, (struct sockaddr *) &p_tAddr, tLen);
    if (0 >= iTmp)
    {
      fprintf(stderr, "Unable to send to address %s: %s\n", inet_ntoa(p_tAddr.sin_addr), strerror(errno));
    }
    else
    {
      bRet = true;
    }
  }

  return bRet;
}

bool CCNRegMsg::hasEntry(const CCNRegEntry& _entry) const
{
	map<string, CCNRegEntry>::const_iterator it;
	it = m_entryMap.find(_entry.getKey());

	return it != m_entryMap.end();
}
