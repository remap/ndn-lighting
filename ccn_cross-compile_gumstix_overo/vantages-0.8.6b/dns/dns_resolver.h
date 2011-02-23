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

#ifndef _DNS_RESOLVER_H
#define _DNS_RESOLVER_H

#include <netinet/in.h>

#include <string>
#include <map>

#include "dns_defs.h"

class DnsPacket;
class DnsTask;
class DnsTsig;

class DnsResolver
{
  public:
    typedef std::map<std::string, DnsTask *> DnsTaskMap_t;
    typedef DnsTaskMap_t::iterator DnsTaskIter_t;
  // Member Variables
  private:
    int m_iSocket;
    int m_iRetries;
    int m_iTimeout;
    uint16_t m_uBuffSize;
    bool m_bDO;
    DnsTsig *m_pTsig;
    struct sockaddr_in m_tRemoteAddr;

    int m_iConc;
    DnsTaskMap_t m_oTaskMap;

    static sockaddr_in s_tDefaultResolverIP;

  // Methods
  public:
    DnsResolver();
    virtual ~DnsResolver();

    uint32_t getNameserver();
    void setNameserver(uint32_t p_uIP);

    uint16_t getPort();
    void setPort(uint16_t p_uPort);

    int getRetries();
    void setRetries(int p_iRetries);

    int getTimeout();
    void setTimeout(int p_iSeconds);

    uint16_t getBuffSize();
    void setBuffSize(uint16_t p_uBuffSize);

    bool getDO();
    void setDO(bool p_bDO);

    bool usingTsig();
    bool setTsig(char *p_szKeyFile);
    bool setTsig(const char *p_kszKeyFile);
    bool setTsig(std::string &p_sKeyFile);
    bool setTsig(std::string &p_sName, u_char *p_pKey, size_t p_uKeyLen);
    void clearTsig();

    bool send(std::string &p_sName, DnsPacket &p_oAnswer);
    bool send(std::string &p_sName, int p_iType, DnsPacket &p_oAnswer);
    bool send(std::string &p_sName, int p_iType, int p_iClass, DnsPacket &p_oAnswer);
    bool send(DnsPacket &p_oQueryPkt, DnsPacket &p_oAnswer);

    int getConcurrency();
    void setConcurrency(int p_iConc);
    bool hasRoomToSend();
    bool hasTasks();
    bool send(DnsTask *p_pTask);
    DnsTask *recv();
    DnsTask *checkForTimeouts();
    bool enqueueTask(DnsTask &p_oTask);
    DnsTask *dequeueTask(DnsPacket &p_oPkt, uint32_t p_uIP);
    static std::string makeKey(DnsPacket &p_oPkt, uint32_t p_uIP);

    static bool init();

  protected:
    bool setNonblocking(int p_iFD);
    int recvAll(u_char *p_pBuff, int p_iBuffLen, struct sockaddr_in &p_tRespAddr, socklen_t &p_tRespLen);
};

#endif
