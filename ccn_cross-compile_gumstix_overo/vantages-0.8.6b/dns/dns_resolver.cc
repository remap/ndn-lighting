/*
 * Copyright (c) 2008,2009, University of California, Los Angeles and 
 * Colorado State University All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *     * Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of NLnetLabs nor the names of its
 *       contributors may be used to endorse or promote products derived from this
 *       software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 * 
 */

#include "config.h"
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <poll.h>
#include <stdlib.h>
#include <algorithm>

#include <iostream>
#include <sstream>

#include "dns_resolver.h"
#include "dns_packet.h"
#include "dns_rr.h"
#include "dns_opt.h"
#include "dns_name.h"
#include "dns_task.h"
#include "dns_tsig.h"
#include "dns_err.h"

using namespace std;

struct sockaddr_in DnsResolver::s_tDefaultResolverIP = {0};

DnsResolver::DnsResolver()
  : m_iSocket(-1),
    m_iRetries(3),
    m_iTimeout(10),
    m_uBuffSize(512),
    m_bDO(false),
    m_pTsig(NULL),
    m_iConc(10)
{
  if (0 == s_tDefaultResolverIP.sin_addr.s_addr)
  {
    if (!init())
    {
      DnsError::getInstance().setError("Unable to initialize resolver.");
      dns_log("Unable to initialize resolver.\n");
    }
    srand(time(NULL));
  }

  if (-1 == (m_iSocket = socket(AF_INET, SOCK_DGRAM, 0)))
  {
    dns_log("Unable to create socket: %s\n", strerror(errno));
    DnsError::getInstance().setError("Unable to create socket");
  }
  else if (!setNonblocking(m_iSocket))
  {
    dns_log("Unable to make socket non-blaocking: %s\n", strerror(errno));
    DnsError::getInstance().setError("Unable to make socket non-blocking");
  }
  memcpy(&m_tRemoteAddr, &s_tDefaultResolverIP, sizeof(m_tRemoteAddr));
}

DnsResolver::~DnsResolver()
{
  if (-1 != m_iSocket)
  {
    close(m_iSocket);
    m_iSocket = -1;
  }

  clearTsig();
}

uint32_t DnsResolver::getNameserver()
{
  return ntohl(m_tRemoteAddr.sin_addr.s_addr);
}

void DnsResolver::setNameserver(uint32_t p_uIP)
{
  m_tRemoteAddr.sin_addr.s_addr = htonl(p_uIP);
}

uint16_t DnsResolver::getPort()
{
  return ntohs(m_tRemoteAddr.sin_port);
}

void DnsResolver::setPort(uint16_t p_uPort)
{
  m_tRemoteAddr.sin_port = htons(p_uPort);
}

int DnsResolver::getRetries()
{
  return m_iRetries;
}

void DnsResolver::setRetries(int p_iRetries)
{
  m_iRetries = p_iRetries;
}

int DnsResolver::getTimeout()
{
  return m_iTimeout;
}

void DnsResolver::setTimeout(int p_iSeconds)
{
  m_iTimeout = p_iSeconds;
}

uint16_t DnsResolver::getBuffSize()
{
  return m_uBuffSize;
}

void DnsResolver::setBuffSize(uint16_t p_uBuffSize)
{
  m_uBuffSize = p_uBuffSize;
}

bool DnsResolver::getDO()
{
  return m_bDO;
}

void DnsResolver::setDO(bool p_bDO)
{
  m_bDO = p_bDO;
}

bool DnsResolver::usingTsig()
{
  return (NULL != m_pTsig);
}

bool DnsResolver::setTsig(char *p_szKeyFile)
{
  return setTsig((const char *) p_szKeyFile);
}

bool DnsResolver::setTsig(const char *p_kszKeyFile)
{
  bool bRet = false;

  if (NULL == p_kszKeyFile)
  {
    clearTsig();
  }
  else
  {
    string sFile = p_kszKeyFile;
    bRet = setTsig(sFile);
  }

  return bRet;
}

bool DnsResolver::setTsig(std::string &p_sKeyFile)
{
  bool bRet = false;

  DnsTsig *pTsig = new DnsTsig;
  if (!pTsig->loadKeyFromFile(p_sKeyFile))
  {
    dns_log("Unable to set TSIG in resolver because could not load key from file.\n");
    delete pTsig;
  }
  else
  {
    if (NULL != m_pTsig)
    {
      delete m_pTsig;
      m_pTsig = NULL;
    }

    m_pTsig = pTsig;
    bRet = true;
  }

  return bRet;
}

bool DnsResolver::setTsig(std::string &p_sKeyName, u_char *p_pKey, size_t p_uKeyLen)
{
  bool bRet = false;

  if (NULL == p_pKey)
  {
    dns_log("Unable to set NULL key.\n");
  }
  else if (0 == p_uKeyLen)
  {
    dns_log("Unable to set key with 0 len\n");
  }
  else
  {
    DnsTsig *pTsig = new DnsTsig();
    DnsName oName(p_sKeyName);
    pTsig->set_name(oName);
    pTsig->setKey(p_pKey, p_uKeyLen);

    if (NULL != m_pTsig)
    {
      delete m_pTsig;
      m_pTsig = NULL;
    }

    m_pTsig = pTsig;
    bRet = true;
  }

  return bRet;
}

void DnsResolver::clearTsig()
{
  if (NULL != m_pTsig)
  {
    delete m_pTsig;
    m_pTsig = NULL;
  }
}

bool DnsResolver::send(std::string &p_sName, DnsPacket &p_oAnswer)
{
  return send(p_sName, DNS_RR_A, p_oAnswer);
}

bool DnsResolver::send(std::string &p_sName, int p_iType, DnsPacket &p_oAnswer)
{
  return send(p_sName, p_iType, DNS_CLASS_IN, p_oAnswer);
}

bool DnsResolver::send(std::string &p_sName, int p_iType, int p_iClass, DnsPacket &p_oAnswer)
{
  bool bRet = false;

  DnsPacket oQuestionPkt(true, -1);
  DnsName oName(p_sName);
  DnsRR *pQuestionRR = DnsRR::question(oName, p_iType);
  if (NULL == pQuestionRR)
  {
    dns_log("Unable to create question for name: '%s' of type: %d\n", p_sName.c_str(), p_iType);
    DnsError::getInstance().setError("Unable to create question for name");
  }
  else if (getBuffSize() > DNS_MAX_PACKET_SIZE)
  {
    dns_log("Unable to send packet size %u (larger that max size: %u)\n", getBuffSize(), DNS_MAX_PACKET_SIZE);
    DnsError::getInstance().setError("Unable to send packet size ");
  }
  else
  {
    pQuestionRR->set_class(p_iClass);
    oQuestionPkt.addQuestion(*pQuestionRR);

    bRet = send(oQuestionPkt, p_oAnswer);
  }

  return bRet;
}

bool DnsResolver::send(DnsPacket &p_oQueryPkt, DnsPacket &p_oAnswer)
{
  bool bRet = false;

  bool bTsig = false;

  if (usingTsig() && NULL == p_oQueryPkt.getTsig())
  {
    DnsTsig *pTsig = new DnsTsig(*m_pTsig);
    p_oQueryPkt.setTsig(pTsig);
    bTsig = true;
  }

  if (getBuffSize() > 512 || getDO() || bTsig)
  {
    DnsOpt *pOptRR = new DnsOpt();
    pOptRR->setMax(getBuffSize());
    pOptRR->setDO(getDO());
    p_oQueryPkt.addAdditional(*pOptRR);
  }

  int iLen = 0;
  size_t uQuestionLen = 0;
  u_char pBuff[DNS_MAX_PACKET_SIZE];

  struct sockaddr_in tRespAddr;
  socklen_t tRespLen = sizeof(tRespAddr);
  memset(&tRespAddr, 0, sizeof(tRespAddr));
  p_oAnswer.clear();

  if (0 == (uQuestionLen = p_oQueryPkt.toWire(pBuff, DNS_MAX_PACKET_SIZE)))
  {
    dns_log("Unable to convert packet to wire format.\n");
    DnsError::getInstance().setError("Unable to convert packet to wire format.");
  }
//  else if (-1 == (m_iSocket = socket(AF_INET, SOCK_DGRAM, 0)))
  else if (-1 == m_iSocket)
  {
    dns_log("Unable to create socket: %s\n", strerror(errno));
    DnsError::getInstance().setError("Unable to create socket");
  }
  else
  {
    struct timeval tSleep;
    fd_set tReads;

    bool bSend = true;
    for (int i = 0; i < m_iRetries; i++)
    {
      if (bSend && -1 == sendto(m_iSocket, pBuff, uQuestionLen, 0, (struct sockaddr *) &m_tRemoteAddr, sizeof(m_tRemoteAddr)))
      {
        dns_log("Unable to send query: %s\n", strerror(errno));
        DnsError::getInstance().setError("Unable to send query");
      }

      bSend = true;
      tSleep.tv_sec = m_iTimeout;
      tSleep.tv_usec = 0;

      FD_ZERO(&tReads);
      FD_SET(m_iSocket, &tReads);

      int iReady = select(m_iSocket + 1, &tReads, NULL, NULL, &tSleep);
      if (iReady < 0 && EINTR != errno)
      {
        dns_log("Unable to select: '%s'\n", strerror(errno));
        DnsError::getInstance().setError("Unable to select");
        break;
      }
      else if (FD_ISSET(m_iSocket, &tReads))
      {
        if (0 >= (iLen = recvfrom(m_iSocket, pBuff, DNS_MAX_PACKET_SIZE, 0, (struct sockaddr *) &tRespAddr, &tRespLen)))
        {
          dns_log("Unable to recvfrom: %s\n", strerror(errno));
          DnsError::getInstance().setError("Unable to recvfrom");
        }
        else if (tRespAddr.sin_addr.s_addr != m_tRemoteAddr.sin_addr.s_addr)
        {
          dns_log("Received response from >different< nameserver than queries! (%u != %u)\n",
                  tRespAddr.sin_addr.s_addr,
                  m_tRemoteAddr.sin_addr.s_addr);
          DnsError::getInstance().setError("Received response from >different< nameserver than queries!");
          i--;
          bSend = false;
        }
        else if (!p_oAnswer.fromWire(pBuff, iLen))
        {
          dns_log("Unable to construct reponse packet from buffer.\n");
          DnsError::getInstance().setError("Unable to construct reponse packet from buffer.");
        }
        else if (p_oAnswer.getHeader().id() != p_oQueryPkt.getHeader().id())
        {
          dns_log("Received response with >different< query ID! (%d != %d)\n", p_oAnswer.getHeader().id(), p_oQueryPkt.getHeader().id());
          DnsError::getInstance().setError("Received response with >different< query ID!");
          i--;
          bSend = false;
        }
        else
        {
dns_log("PROCESSING ANSWER.\n");
          if (NULL != p_oQueryPkt.getTsig())
          {
            if (NULL == p_oAnswer.getTsig())
            {
              dns_log("Query had TSIG, but answer does not.\n");
              DnsError::getInstance().setError("Query had TSIG, but answer does not.");
            }
            else if (!p_oAnswer.getTsig()->verify(p_oAnswer, p_oQueryPkt.getTsig()->getMac(), p_oQueryPkt.getTsig()->getMacSize()))
            {
              dns_log("TSIG in answer does not verify.\n");
            }
            else
            {
              dns_log("Successfully verified TSIG.\n");
              bRet = true;
            }
          }
          else
          {
            bRet = true;
          }
          break;
        }
      }
    }
  }

  return bRet;
}

int DnsResolver::getConcurrency()
{
  return m_iConc;
}

void DnsResolver::setConcurrency(int p_iConc)
{
  m_iConc = p_iConc;
}

bool DnsResolver::hasRoomToSend()
{
  bool bRet = false;


  if (m_oTaskMap.size() < (unsigned) m_iConc)
  {
    struct pollfd tPoll;
    tPoll.fd = m_iSocket;
    tPoll.events = POLLOUT;
    tPoll.revents = 0;
    int iErr = poll(&tPoll, 1, 200);
    if (tPoll.revents & POLLERR)
    {
      dns_log("Error on socket");
      DnsError::getInstance().setError("Error on socket");
    }
    else if (tPoll.revents & POLLHUP)
    {
      dns_log("Socket hang up");
      DnsError::getInstance().setError("Socket hang up");
    }
    else if (tPoll.revents & POLLNVAL)
    {
      dns_log("Socket not open");
      DnsError::getInstance().setError("Socket not open");
    }
    else if (EINTR != errno && 1 == iErr)
    {
      bRet = true;
    }
  }

  return bRet;
}

bool DnsResolver::hasTasks()
{
  return !m_oTaskMap.empty();
}

bool DnsResolver::send(DnsTask *p_pTask)
{
  bool bRet = false;

  size_t uQuestionLen = 0;
  u_char pBuff[DNS_MAX_PACKET_SIZE];

  DnsPacket *pQuery = NULL;

  struct sockaddr_in tAddr;
  memcpy(&tAddr, &m_tRemoteAddr, sizeof(tAddr));

  if (NULL == p_pTask)
  {
    dns_log("Task cannot be NULL\n");
    DnsError::getInstance().setError("Task cannot be NULL");
  }
  else if (NULL == (pQuery = p_pTask->getQuery()))
  {
    dns_log("Query packet cannot be NULL\n");
    DnsError::getInstance().setError("Query packet cannot be NULL");
  }
  else if (!hasRoomToSend())
  {
    dns_log("No room left for new query.  Size is: %u\n", (unsigned) m_oTaskMap.size());
    DnsError::getInstance().setError("No room left for new query");
  }
  else
  {
    if (0 != p_pTask->getNameserver())
    {
      tAddr.sin_addr.s_addr = htonl(p_pTask->getNameserver());
    }

    if (0 != p_pTask->getPort())
    {
      tAddr.sin_port = htons(p_pTask->getPort());
    }

    if (getBuffSize() > 512 || getDO())
    {
      DnsOpt *pOptRR = new DnsOpt();
      pOptRR->setMax(getBuffSize());
      pOptRR->setDO(getDO());
      pQuery->addAdditional(*pOptRR);
    }

    if (-1 == m_iSocket)
    {
      dns_log("Unable to create socket: %s\n", strerror(errno));
      DnsError::getInstance().setError("Unable to create socket");
    }
    // First enqueue the task to make sure we have room, and to remap the 
    // qid if there is a conflict.
    else if (!enqueueTask(*p_pTask))
    {
      dns_log("Unable to enqueue task.\n");
      DnsError::getInstance().setError("Unable to enqueue task.");
    }
    // Now that we have the task all set, convert it to wire format
    // and send it off.
    else if (0 == (uQuestionLen = pQuery->toWire(pBuff, DNS_MAX_PACKET_SIZE)))
    {
      dns_log("Unable to convert packet to wire format.\n");
    }
    else if (-1 == sendto(m_iSocket, pBuff, uQuestionLen, 0, (struct sockaddr *) &tAddr, sizeof(tAddr)))
    {
      dns_log("Unable to send query: %s\n", strerror(errno));
      DnsError::getInstance().setError("Unable to send query");
    }
    else
    {
      time_t tTimeout = time(NULL) + m_iTimeout;
      p_pTask->setTimeout(tTimeout);
      bRet = true;
    }
  }

  return bRet;
}

DnsTask *DnsResolver::recv()
{
  DnsTask *pRet = NULL;

  int iLen = 0;

  struct pollfd tPoll;
  tPoll.fd = m_iSocket;
  tPoll.events = POLLIN;
  tPoll.revents = 0;
  int iErr = 0;

  struct sockaddr_in tRespAddr;
  socklen_t tRespLen = sizeof(tRespAddr);
  memset(&tRespAddr, 0, sizeof(tRespAddr));

  if (!hasTasks())
  {
    dns_log("Unable to recv() w/o any tasks enqueued.\n");
    DnsError::getInstance().setError("Unable to recv() w/o any tasks enqueued.");
  }
  else if (-1 == (iErr = poll(&tPoll, 1, 5)))
  {
    dns_log("Unable to poll FD: %d: [%d] %s\n", m_iSocket, iErr, strerror(errno));
    DnsError::getInstance().setError("Unable to poll FD");
  }
  else if (tPoll.revents & POLLERR)
  {
    dns_log("Error on socket\n");
    DnsError::getInstance().setError("Error on socket");
  }
  else if (tPoll.revents & POLLHUP)
  {
    dns_log("Socket hang up\n");
    DnsError::getInstance().setError("Socket hang up");
  }
  else if (tPoll.revents & POLLNVAL)
  {
    dns_log("Socket not open\n");
    DnsError::getInstance().setError("Socket not open");
  }
  else if (EINTR != errno && 1 == iErr)
  {
    DnsPacket *pRespPkt = new DnsPacket();
    u_char pBuff[DNS_MAX_PACKET_SIZE];

    if (0 >= (iLen = recvfrom(m_iSocket, pBuff, DNS_MAX_PACKET_SIZE, 0, (struct sockaddr *) &tRespAddr, &tRespLen)))
    {
      dns_log("Unable to recvfrom: %s\n", strerror(errno));
      DnsError::getInstance().setError("Unable to recvfrom\n");
      delete pRespPkt;
      pRespPkt = NULL;
    }
    else if (!pRespPkt->fromWire(pBuff, iLen))
    {
      dns_log("Unable to construct reponse packet from buffer.\n");
      DnsError::getInstance().setError("Unable to construct reponse packet from buffer.");
      delete pRespPkt;
      pRespPkt = NULL;
    }
    else if (NULL == (pRet = dequeueTask(*pRespPkt, ntohl(tRespAddr.sin_addr.s_addr))))
    {
#ifdef _DNS_DEBUG
      string sBlahKey = makeKey(*pRespPkt, ntohl(tRespAddr.sin_addr.s_addr));
      dns_log("Unable to locate task for packet with key '%s'\n", sBlahKey.c_str());
#endif
      DnsError::getInstance().setError("Unable to locate task for packet with key");
      delete pRespPkt;
      pRespPkt = NULL;
    }
    else
    {
      pRet->setResponse(pRespPkt);
      pRespPkt = NULL;
    }
  }
  else if (NULL != (pRet = checkForTimeouts()))
  {
#ifdef _DNS_DEBUG
    uint32_t uIP = pRet->getNameserver();
    if (0 == uIP)
    {
      uIP = ntohl(m_tRemoteAddr.sin_addr.s_addr);
    }
    string sKey = makeKey(*(pRet->getQuery()), uIP);
    dns_log("Timing out task with key: '%s'\n", sKey.c_str());
#endif
    pRet = pRet;
  }

  return pRet;
}

bool DnsResolver::enqueueTask(DnsTask &p_oTask)
{
  bool bRet = false;

  DnsPacket *pQuery = p_oTask.getQuery();
  if (!hasRoomToSend())
  {
    dns_log("Unable to enqueueTask() when queue has no room.\n");
    DnsError::getInstance().setError("Unable to enqueueTask() when queue has no room.");
  }
  else if (NULL == pQuery)
  {
    dns_log("Cannot enqueue task with NULL query.\n");
    DnsError::getInstance().setError("Cannot enqueue task with NULL query.");
  }
  else
  {
    uint32_t uIP = p_oTask.getNameserver();
    if (0 == uIP)
    {
      uIP = ntohl(m_tRemoteAddr.sin_addr.s_addr);
    }

    for (int i = 0; i < 3 && !bRet; i++)
    {
      string sKey = makeKey(*pQuery, uIP);
      DnsTaskIter_t tIter = m_oTaskMap.find(sKey);
      if (m_oTaskMap.end() != tIter)
      {
        if (i + 1 >= 3)
        {
          dns_log("Entry in queue already exists for key '%s'!  This is an error!\n", sKey.c_str());
          DnsError::getInstance().setError("Entry in queue already exists for key!  This is an error!");
        }
        p_oTask.getQuery()->getHeader().assignID();
      }
      else
      {
        m_oTaskMap[sKey] = &p_oTask;
        bRet = true;
      }
    }
  }

  return bRet;
}

DnsTask *DnsResolver::dequeueTask(DnsPacket &p_oPkt, uint32_t p_uIP)
{
  DnsTask *pRet = NULL;

  string sKey = makeKey(p_oPkt, p_uIP);

  DnsTaskIter_t tIter = m_oTaskMap.find(sKey);
  if (m_oTaskMap.end() != tIter)
  {
    pRet = tIter->second;
    m_oTaskMap.erase(tIter);
  }

  return pRet;
}

DnsTask *DnsResolver::checkForTimeouts()
{
  DnsTask *pRet = NULL;

  time_t tNow = time(NULL);

  for (DnsTaskIter_t tIter = m_oTaskMap.begin();
       m_oTaskMap.end() != tIter;
       tIter++)
  {
    DnsTask *pTask = tIter->second;
    if (pTask->getTimeout() < tNow)
    {
      pRet = pTask;
      pRet->setResponse(NULL);
      m_oTaskMap.erase(tIter);
      break;
    }
  }

  return pRet;
}

std::string DnsResolver::makeKey(DnsPacket &p_oPkt, uint32_t p_uIP)
{
  string sRet;

  ostringstream oSS;
  RRList_t tQs;
  if (p_oPkt.getHeader().qd_count() != 1)
  {
    dns_log("Improper number of questions: %d\n", p_oPkt.getHeader().qd_count());
    DnsError::getInstance().setError("Improper number of questions");
  }
  else if (!p_oPkt.getQuestions(tQs))
  {
    dns_log("Unable to get question RRs.\n");
    DnsError::getInstance().setError("Unable to get question RRs.");
  }
  else if (tQs.empty())
  {
    dns_log("Question list is empty, but QD count is == 1\n");
    DnsError::getInstance().setError("Question list is empty, but QD count is == 1");
  }
  else
  {
    DnsRR *pRR = tQs.front();
    string sName = pRR->get_name()->toString();
    transform(sName.begin(), sName.end(), sName.begin(), ::tolower);
    oSS << sName
        << "|"
        << pRR->type()
        << "|"
        << pRR->get_class()
        << "|"
        << p_oPkt.getHeader().id()
        << "|"
        << (unsigned) p_uIP;
    sRet = oSS.str();
  }

  return sRet;
}

bool DnsResolver::setNonblocking(int p_iFD)
{
  bool bRet = false;
  int iFlags = 0;

#if defined(O_NONBLOCK)
  if (-1 == (iFlags = fcntl(p_iFD, F_GETFL, 0)))
  {
    iFlags = 0;
  }

  bRet = (0 == fcntl(p_iFD, F_SETFL, iFlags | O_NONBLOCK));
#else
  iFlags = 1;
  bRet = (0 == ioctl(p_iFD, FIOBIO, &iFlags));
#endif

  return bRet;
}

int DnsResolver::recvAll(u_char *p_pBuff, int p_iBuffLen, struct sockaddr_in &p_tRespAddr, socklen_t &p_tRespLen)
{
  int iRet = -1;

  if (-1 == m_iSocket)
  {
    dns_log("Socket DNE\n");
    DnsError::getInstance().setError("Socket DNE");
  }
  else if (NULL == p_pBuff)
  {
    dns_log("Must specifyu buffer.\n");
    DnsError::getInstance().setError("Must specifyu buffer.");
  }
  else if (p_iBuffLen <= 0)
  {
    dns_log("Buffer len must be greater than 0 (not %d).\n", p_iBuffLen);
    DnsError::getInstance().setError("Buffer len must be greater than 0");
  }
  else
  {
    int iLen = 0;

    iRet = 0;
    while (0 != (iLen = recvfrom(m_iSocket, &(p_pBuff[iLen]), p_iBuffLen - iLen, 0, (struct sockaddr *) &p_tRespAddr, &p_tRespLen)))
    {
      if (iLen < 0 && EINTR != errno)
      {
        dns_log("Got error on socket %d: %s\n", m_iSocket, strerror(errno));
        DnsError::getInstance().setError("Got error on socket");
        iRet = -1;
        break;
      }
      else
      {
        iRet += iLen;
      }
    }
  }

  return iRet;
}

bool DnsResolver::init()
{
  bool bRet = false;

  FILE *pFile = NULL;

  if (NULL == (pFile = fopen("/etc/resolv.conf", "r")))
  {
    dns_log("Unable to open '/etc/resolv.conf': %s\n", strerror(errno));
  }
  else
  {
    unsigned uA = 0;
    unsigned uB = 0;
    unsigned uC = 0;
    unsigned uD = 0;

    s_tDefaultResolverIP.sin_addr.s_addr = 0;

    char pBuff[BUFSIZ];
    memset(pBuff, 0, BUFSIZ);
    while (fgets(pBuff, BUFSIZ, pFile))
    {
      if (sscanf(pBuff, "nameserver %u.%u.%u.%u", &uA, &uB, &uC, &uD))
      {
        s_tDefaultResolverIP.sin_addr.s_addr = htonl(
                                             (uA << 24)
                                             + (uB << 16)
                                             + (uC << 8)
                                             + uD
                                           );
        s_tDefaultResolverIP.sin_port = htons(53);
        s_tDefaultResolverIP.sin_family = AF_INET;
        break;
      }
    }
    fclose(pFile);
    dns_log("Setting default resolver to: '%s'\n", inet_ntoa(s_tDefaultResolverIP.sin_addr));
    bRet = true;
  }

  return bRet;
}
