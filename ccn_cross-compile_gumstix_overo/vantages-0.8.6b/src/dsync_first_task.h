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

#ifndef _DSYNC_FIRST_TASK_H
#define _DSYNC_FIRST_TASK_H

#include <string>
#include <vector>

#include "dsync_defs.h"
#include "dns_task.h"
#include "ps_task.h"
#include "dns_name.h"
#include "dns_ds.h"
#include "dns_dnskey.h"
#include "dns_verifier.h"
#include "dsync_zone_dao.h"
#include "dsync_rrset_dao.h"
#include "dsync_ns_dao.h"
#include "ps_poll_task.h"
#include "dsync_dnskey_abstraction_task.h"

class DsyncFirstTask : public DsyncDnskeyAbstractionTask 
{
  // Member Variables
  private:
    DsyncZoneDao* zDao;
    std::vector<std::string> NsList;
    std::vector<uint32_t> uNsB;
    std::vector<uint32_t> vNsB;
    std::vector<uint32_t> NsB;
    //NsVector Ns;
    uint32_t validNameserver;
    std::string validNameserverName;
    std::string validNameserverIp;
    //NsVector uNs;
    //NsVector vNs;
    std::string privateName;
    DnsVerifier verifier;
    RRList_t keys;
    RRList_t keySigs;
    RRList_t Ds;
    RRList_t DsSigs;
    int m_iRound;
    int syncState;
    float weighted_check;
    bool checked;
  
  // Methods
  public:
    DsyncFirstTask(DsyncZoneDao* zD, float wcheck);
    virtual ~DsyncFirstTask();

    virtual bool execute();
    virtual bool done();
    virtual bool process();

  protected:
    bool prepareDnskey();
    bool prepareNDnskey();
    bool processDnskey();
    bool processNDnskey();
    bool prepareNs();
    bool processNs();
    bool prepareA();
    bool processA(); 
    bool prepareBlanketNs();
    bool prepareDs();
    bool prepareBlanketA();    
    bool processBlanketNs();
    bool processDs();
    bool processBlanketA();
    void serialize();
    void sleepTTL();
    bool prepareA(std::string n);
    bool processA(std::string n);
    bool prepareNsC();
    bool processNsC();
    bool prepareAC();
    bool processAC();
};

#endif
