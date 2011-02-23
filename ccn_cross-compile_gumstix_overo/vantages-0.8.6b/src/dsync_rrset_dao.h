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

#ifndef _DSYNC_RRSET_DAO_H
#define _DSYNC_RRSET_DAO_H

#include <string>

#include "ps_dao.h"
#include "ps_defs.h"
#include "dns_dnskey.h"
#include "dns_ds.h"
#include "dns_defs.h"
#include "dns_rrsig.h"
#include "dsync_ns_dao.h"
#include "dsync_defs.h"
#include "ps_config.h"


class DsyncRrsetDao : public PsDao
{
  // Member Variables
  public:
  std::string zoneName;
  //select/update/delete strings go here
  static const char* selectKeyset;
  static const char* insertKey;
  static const char* deleteRRSet;
  static const char* getExpired;
  private:
  RRList_t v_oDs;
  RRList_t v_oDnskey; 
  RRList_t v_oSig;
  std::string nsName;
  std::string nsIp;
  int iDatabaseID;
 
  // Methods
  public:
  DsyncRrsetDao();
  virtual ~DsyncRrsetDao();

  virtual bool serialize();
  virtual bool deserialize();
  virtual bool deserialize(DaoList_t &p_oOutputList);

  virtual DsyncRrsetDao *dup();
  virtual std::string daoName();

  void addKey(DnsDnskey * key);
  bool deleteKey(DnsDnskey * key);
  void addSig(DnsRrsig * sig);
  void addDs(DnsDs * ds);
  bool deleteDs(DnsDs * ds); 
  bool setKeyset(RRList_t keyset);
  bool setDsset(RRList_t DsSet);  
  bool setSigset(RRList_t sigset);
  RRList_t& getKeyset();
  RRList_t& getDsset();
  RRList_t& getSigset();
  void setDatabaseID(int i);
  int getDatabaseID();
  bool setsMatch(RRList_t v1, RRList_t v2); 
  void setSourceNs(std::string n, std::string i);
  
  protected:
  bool serializeDnskey();
  bool serializeDs();
  bool serializeRrsig();
  
};

#endif
