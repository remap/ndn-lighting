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

#ifndef _DSYNC_ZONE_DAO_H
#define _DSYNC_ZONE_DAO_H

#include <string>

#include "ps_dao.h"
#include "ps_defs.h"
#include "dns_dnskey.h"
#include "dns_ds.h"
#include "dns_defs.h"
#include "dsync_rrset_dao.h"
#include "dsync_ns_dao.h"
#include "dsync_defs.h"
#include "ps_config.h"

class DsyncZoneDao : public PsDao
{
  // Member Variables
  public:
  std::string zoneName;
  std::string parentName;

  private:
  std::vector<DsyncRrsetDao* > Rrsets;
  DsyncNsDao Nsset;
  int syncState;
  
  // Methods
  public:
  DsyncZoneDao();
  virtual ~DsyncZoneDao();

  virtual bool serialize();
  virtual bool serialize(int sTime);
  virtual bool deserialize();
  virtual bool deserialize(DaoList_t &p_oOutputList);

  virtual DsyncZoneDao *dup();
  virtual std::string daoName();

  void addKeyset(RRList_t keys, RRList_t sigs);
  void addKeyset(RRList_t keys, RRList_t sigs, std::string name, std::string ip);
  void addDsset(RRList_t dss, RRList_t sigs);
  void addSigset(RRList_t sigs);
  std::vector<DsyncRrsetDao*> getRrsets();
  void addNs(std::string name, std::string ip, bool verified);
  DsyncNsDao & getNsDao();
  void setLastNs(std::string name, std::string ip);
  int getSyncState();
  void setSyncState(int s);
  RRList_t getKeyset();
  void printAll();
  //void flushDatabase();  --dangerous function, here for development convenience 
  bool deleteSelf();
  uint32_t getDnskeyTTL();
  uint32_t getDsTTL();
  static void clearList(RRList_t &p_oList);

};

#endif
