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

#ifndef _DNS_POLLER_H
#define _DNS_POLLER_H

#include <sys/types.h>
#include <string>

#include "ps_dao.h"
#include "i_ps_poller.h"
#include "dns_resolver.h"

class DnskeyPollTask;

class DnsPollerDao : public PsDao, public IPsPoller
{
  // Member Variables
  public:
    static const char *s_kszDaoName;
    static const char *s_szSelect;
    static const char *s_szSelectNextTime;
    static const char *s_kszUpdateList;

  private:
    int m_iPollPeriod;
    std::string m_sName;
    UrlList_t m_tList;
    DaoList_t m_oDaoList;
    DnsResolver m_oRes;

  // Methods
  public:
    DnsPollerDao();
    virtual ~DnsPollerDao();

    virtual bool serialize();
    virtual bool deserialize();
    virtual bool deserialize(DaoList_t &p_oOutputList);
    virtual std::string daoName();
    virtual DnsPollerDao *dup();

    virtual bool poll();
    virtual bool extract(DnskeyPollTask &p_oTask);

    UrlIter_t begin();
    UrlIter_t end();
    size_t size();
    bool empty();

    DaoIter_t beginDao();
    DaoIter_t endDao();
    size_t sizeDao();
    bool emptyDao();

    bool clear();

  private:
//    bool parseURL(std::string &p_sURL);
};

#endif
