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

#ifndef _DNSKEY_APP_H
#define _DNSKEY_APP_H

#include <sys/types.h>
#include <string>
#include <list>

#include "dnskey_pcap_loader.h"
#include "dnskey_defs.h"
#include "ps_dao.h"
#include "ps_app.h"
#include "ps_poller.h"
#include "dns_resolver.h"
#include "ps_defs.h"
#include "ps_mutex.h"

class DnskeyPollTask;
class PsThreadPool;

class DnskeyApp : public PsDao, public PsApp
{
  // Member Variables
  public:
    static const char *s_kszDaoName;
    static const char *s_szSelect;
    static const char *s_szSelectNextTime;
    static const char *s_kszUpdateList;
    static const char *s_kszInsertMonTarget;
    static const char *s_kszInsertRunStats;
    static const char *s_kszClearStaleMons;
    static const char *s_kszClearStaleProcSelfRel;
    static const char *s_kszClearStaleProcRawRel;
    static const char *s_kszClearStaleProc;
    static const char *s_kszClearStaleRaw;


  private:
    bool m_bInit;
    int m_iPollPeriod;
    time_t m_tStartTime;
    time_t m_tStopTime;
    std::string m_sName;
    UrlList_t m_tList;
    UrlList_t m_tUrlList;
    PsPollTaskList_t m_oTaskList;
    DnsResolver m_oRes;
    PsMutex m_oHopperMutex;
    std::list<DnskeyPollTask *> m_oHopperList;
    PsPoller m_oPoller;
    DnskeyPcapLoader m_oPcap;

  // Methods
  public:
    DnskeyApp();
    virtual ~DnskeyApp();

    bool load();

    virtual bool serialize();
    virtual bool deserialize();
    virtual bool deserialize(DaoList_t &p_oOutputList);
    virtual std::string daoName();
    virtual DnskeyApp *dup();

    UrlIter_t begin();
    UrlIter_t end();
    size_t size();
    bool empty();

    UrlIter_t beginUrl();
    UrlIter_t endUrl();
    size_t sizeUrl();
    bool emptyUrl();

    PsPollTaskIter_t beginTask();
    PsPollTaskIter_t endTask();
    size_t sizeTask();
    bool emptyTask();

    int getPollPeriod();

    bool clear();

    virtual const char *getHttpPath();
    virtual const char *getHttpPass();
    virtual HttpListenerCtx *createCtx();
    virtual bool enabled();

    bool addNewTarget(std::string &p_sName, std::string &p_sURL, int p_iScraperID, dnskey_src_e p_eSrc, int p_iScraperParamID = -1);

    virtual bool init();
    virtual bool execute();

  private:
//    bool parseURL(std::string &p_sURL);
};

#endif
