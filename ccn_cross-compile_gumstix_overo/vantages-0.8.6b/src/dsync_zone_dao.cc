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

#include "ps_logger.h"
#include <stdio.h>

#include <sstream>
#include <iostream>

#include "dsync_zone_dao.h"
#include "ps_defs.h"
#include "dao_factory.h"
#include "raw_data_dao.h"

using namespace std;

DsyncZoneDao::DsyncZoneDao() : zoneName(""), parentName(""), syncState(-1)
{
  //string cs = "/home3/tuchsche/dsync.db";
  //setConnectString(cs);
}

DsyncZoneDao::~DsyncZoneDao()
{
  std::vector<DsyncRrsetDao*>::iterator it;
  for (it = Rrsets.begin(); it != Rrsets.end(); it++)
  {
    delete *it;
  }
  Rrsets.clear();
}

bool DsyncZoneDao::serialize()
{
  int sleepTime = 5;
  return serialize(sleepTime);
}

bool DsyncZoneDao::serialize(int sTime)
{
  if (!connect(__FILE__, __LINE__))
  {
    ps_elog(PSL_ERROR, "Unable to connect: '%s'\n", getError());
  }
  else if (!prepareQuery("Select ZoneID from DSYNC_ZONE where ZoneName = ?"))
  {
    ps_elog(PSL_ERROR, "Unable to prepare query: '%s'\n", getError());
  }
  else if (!setStr(0, zoneName))
  {
    ps_elog(PSL_ERROR, "Unable to set zoneName in query: '%s'\n", getError());
  }
  else if (!exec())
  {
    ps_elog(PSL_ERROR, "Unable to execute query: '%s'\n", getError());
  }
  if (!next())
  {
    clearQuery();
    prepareQuery("Insert into DSYNC_ZONE (ZoneName, ZoneAlarm, ParentName, State) VALUES (?, ?, ?, ?)");
    setStr(0, zoneName);
    setInt(1, time(0) + sTime);
    setStr(2, parentName);
    setInt(3, syncState);
    //printf("Adding zone.  Current time:  %ld,   Alarm time:  %ld\n", (long)time(0), (long)(time(0) + sleepTime));
    update();
  }
  else
  {
    clearQuery();
    prepareQuery("Update DSYNC_ZONE set State = ?, ZoneAlarm = ? where ZoneName = ?");
    setInt(0, syncState);
    setInt(1, time(0) + sTime);
    setStr(2, zoneName);
    update();
  }
  clearQuery();
  disconnect();

  bool bRet = true;
  Nsset.zoneName = zoneName;
  Nsset.serialize();
  for (unsigned int i = 0; i < Rrsets.size(); i++)
  {
    Rrsets.at(i)->zoneName = zoneName;
    if(false == Rrsets.at(i)->serialize())
    {
      bRet = false;
    }
  }
  return bRet;
}

bool DsyncZoneDao::deserialize()
{
  //printf("ZoneDao::deserialize() called...\n");
  if (zoneName == "")
  {
    return false;
  }
  connect(__FILE__, __LINE__);
  bool bRet = false;
  prepareQuery("select State, ParentName from DSYNC_ZONE where ZoneName = ?");
  if (!setStr(0, zoneName))
  {
    ps_elog(PSL_CRITICAL, "Unable to set zoneName in query: '%s'\n", getError());
  }
  else if (!exec())
  {
    ps_elog(PSL_CRITICAL, "Unable to execute query: '%s'\n", getError());
  }
  else if (next())
  {
    syncState = getInt(0);
    getStr(1, parentName);
    //printf("Deserialized zone: state: %d, parentName = %s\n", syncState, parentName.c_str());
  }
  else
  {
    ps_elog(PSL_DEBUG, "No results returned for query in DsyncZoneDao::deserialize()\n");
    clearQuery();
    disconnect();
    return false; 
  }
  clearQuery();
  
  //get just the most recent dnskey set
  if (!prepareQuery("select max (RRSetFK) from DSYNC_DNSKEY where RRSetFK in (select RRSetID from DSYNC_RRSET r inner join DSYNC_ZONE z on r.zonefk = z.zoneid where ZoneName = ?)"))
  {
    ps_elog(PSL_ERROR, "Unable to prepare query to select most recent dnskey's rrsetfk: %s\n", getError());
  }
  else if (! setStr(0, zoneName))
  {
    ps_elog(PSL_ERROR, "Unable to set zoneName in query: %s'\n", getError());
  }
  else if (!exec())
  {
    ps_elog(PSL_ERROR, "Unable to execute query: %s'\n", getError());
  }
  if (!next())
  {
    ps_elog(PSL_DEBUG, "No results: %s\n", getError());
    clearQuery();
    disconnect();
  }
  else
  {
    int keyRrId = getInt(0);
    
    clearQuery();
    disconnect();

    DsyncRrsetDao* keyDao = new DsyncRrsetDao();
    keyDao->zoneName = zoneName;
    keyDao->setDatabaseID(keyRrId);
    keyDao->deserialize();
    Rrsets.push_back(keyDao);
  } 

  if (!connect(__FILE__, __LINE__))
  {
    ps_elog(PSL_CRITICAL, "Unable to connect(): '%s'\n", getError());
  }
  //get just the most recent ds set
  else if (!prepareQuery("select max (rrsetfk) from DSYNC_DS where rrsetfk in (select RRSetID from DSYNC_RRSET r inner join DSYNC_ZONE z on r.zonefk = z.zoneid where ZoneName = ?)"))
  {
    ps_elog(PSL_ERROR, "Unable to prepare query to select most recent dnskey's rrsetfk: %s\n", getError());
  }
  else if (!setStr(0, zoneName))
  {
    ps_elog(PSL_ERROR, "Unable to prepare query to select most recent dnskey's rrsetfk: %s\n", getError());
  }
  else if (!exec())
  {
    ps_elog(PSL_ERROR, "Unable to prepare query to select most recent dnskey's rrsetfk: %s\n", getError());
  }
  if (!next())
  {
    ps_elog(PSL_DEBUG, "No results: %s\n", getError());
    clearQuery();
    disconnect();
  }
  else
  {
    int dsRrId = getInt(0);
    clearQuery();
    disconnect();

    DsyncRrsetDao *dsDao = new DsyncRrsetDao();
    dsDao->zoneName = zoneName;
    dsDao->setDatabaseID(dsRrId);
    dsDao->deserialize();
    Rrsets.push_back(dsDao);
  }

  Nsset.zoneName = zoneName;
  Nsset.deserialize();
 
  bRet = true;
  return bRet;
}

bool DsyncZoneDao::deserialize(DaoList_t &p_oOutputList)
{
  bool bRet = false;
  PsDao::clearList(p_oOutputList);
  std::list<std::string> zonenames;
  
  if (!connect(__FILE__, __LINE__))
  {
    ps_elog(PSL_CRITICAL, "Unable to connect(): '%s'\n", getError());
  }
  else if (!prepareQuery("Select ZoneName from DSYNC_ZONE where (? > ZoneAlarm) and (State > -1)"))
  {
    ps_elog(PSL_CRITICAL, "Unable to prepare query: '%s'\n", getError());
  }
  else if (!setInt(0, time(0)))
  {
    ps_elog(PSL_CRITICAL, "Unable to set time(): '%s'\n", getError());
  }
  else if (!exec())
  {
    ps_elog(PSL_CRITICAL, "Unable to exectue query: '%s'\n", getError());
  }
  while (next())
  {
    std::string tname;
    getStr(0, tname);
    zonenames.push_back(tname);
  }

  clearQuery();
  disconnect();

  for (std::list<std::string>::iterator it = zonenames.begin();  it != zonenames.end(); it++)
  {
    DsyncZoneDao* tDao = new DsyncZoneDao();
    tDao->zoneName = *it;
    tDao->deserialize();
    p_oOutputList.push_back(tDao);
  }

  return bRet;
}

DsyncZoneDao *DsyncZoneDao::dup()
{
  return new DsyncZoneDao();
}

std::string DsyncZoneDao::daoName()
{
  return "";
}

void DsyncZoneDao::addKeyset(RRList_t keys, RRList_t sigs)
{
  std::vector<DsyncRrsetDao*> RrsetsCopy = Rrsets;
  int removed = 0;
  for (unsigned int i = 0; i < RrsetsCopy.size(); i++)
  {
    if (!RrsetsCopy.at(i)->getKeyset().empty())
    {
      delete *(Rrsets.begin() +i - removed);
      Rrsets.erase(Rrsets.begin() + i - removed);
      removed++;
    }
  }
  //----

  DsyncRrsetDao* tempDao = new DsyncRrsetDao();
  tempDao->zoneName = zoneName;
  tempDao->setDatabaseID(-1);
  tempDao->setKeyset(keys); 
  tempDao->setSigset(sigs);
  Rrsets.push_back(tempDao);
}

void DsyncZoneDao::addKeyset(RRList_t keys, RRList_t sigs, std::string name, std::string ip)
{
  std::vector<DsyncRrsetDao*> RrsetsCopy = Rrsets;
  int removed = 0;
  for (unsigned int i = 0; i < RrsetsCopy.size(); i++)
  {
    if (!RrsetsCopy.at(i)->getKeyset().empty())
    {
      delete *(Rrsets.begin() +i - removed);
      Rrsets.erase(Rrsets.begin() + i - removed);
      removed++;
    }
  }
 
  DsyncRrsetDao* tempDao = new DsyncRrsetDao();
  tempDao->zoneName = zoneName;
  tempDao->setDatabaseID(-1);
  tempDao->setKeyset(keys); 
  tempDao->setSigset(sigs);
  tempDao->setSourceNs(name, ip);
  Rrsets.push_back(tempDao);

}

void DsyncZoneDao::addDsset(RRList_t dss, RRList_t sigs)
{
  std::vector<DsyncRrsetDao* > RrsetsCopy = Rrsets;
  int removed = 0;
  for (unsigned int i = 0; i < RrsetsCopy.size(); i++)
  {
    if (!RrsetsCopy.at(i)->getDsset().empty())
    {
      delete *(Rrsets.begin() +i - removed);
      Rrsets.erase(Rrsets.begin() + i - removed);
      removed++;
    }
  }

  //printf("Building new RrsetDao\n");
  DsyncRrsetDao* tempDao = new DsyncRrsetDao();
  tempDao->zoneName = zoneName;
  tempDao->setDatabaseID(-1);
  tempDao->setDsset(dss);
  tempDao->setSigset(sigs);
  //printf("Pushing to Rrsets\n");
  Rrsets.push_back(tempDao);
  //printf("Returning\n");
}


std::vector<DsyncRrsetDao*> DsyncZoneDao::getRrsets()
{
  return Rrsets; 
}

void DsyncZoneDao::addNs(std::string name, std::string ip, bool verified)
{
  Nsset.zoneName = zoneName;
  NsObj *nsp = new NsObj();
  nsp->name = name;
  nsp->ip = ip;
  nsp->verified = verified;
  Nsset.addNs(nsp); 
}

DsyncNsDao & DsyncZoneDao::getNsDao()
{
  return Nsset;
}


/*
void DsyncZoneDao::flushDatabase()
{
  printf("Ultimate destruction\n");
  if (!connect(__FILE__, __LINE__))
  {
    ps_elog(PSL_ERROR, "Unable to connect: '%s'\n", getError());
  }
  if (!prepareQuery("delete from dnskey; delete from ds; delete from rrset; delete from zone; delete from nameserver"))
  {
    ps_elog(PSL_ERROR, "Unable to prepare query: '%s'\n", getError());
  }
  if (!update())
  {
    ps_elog(PSL_ERROR, "Unable to run query of ultimate destruction: '%s'\n");
  }
  clearQuery();
}
*/

void DsyncZoneDao::setLastNs(std::string name, std::string ip)
{
  Rrsets.back()->setSourceNs(name, ip);
}

void DsyncZoneDao::setSyncState(int s)
{
  syncState = s;
}

int DsyncZoneDao::getSyncState()
{
  return syncState;
}

RRList_t DsyncZoneDao::getKeyset()
{
  std::vector<DsyncRrsetDao*>::iterator it;
  RRList_t keys;
  for (it = Rrsets.begin(); it != Rrsets.end(); it++)
  {
    //printf("--Rrset:--\n");
    RRList_t tKeys = (*it)->getKeyset();
    for (RRIter_t k = tKeys.begin(); k != tKeys.end(); k++)
    {
    //printf("  .getKeyset() element\n");
      keys.push_back(*k);
    }
  }
  return keys;
}

void DsyncZoneDao::printAll()
{
  std::vector<DsyncRrsetDao*>::iterator iter;
  std::vector<DsyncRrsetDao*> daos = Rrsets;
  printf("\n=================================\n");
  printf(" ~ ~ ~ ~ ~ ~ |DAO|  ~ ~ ~ ~ ~ ~ \n");
  printf("---------------------------------\n");
  printf("Zone: %s\n", zoneName.c_str());
  printf("state: %d\n", syncState);
  printf("parent: %s\n", parentName.c_str());
  printf("---------------------------------\n");
  printf("Nameservers:\n");
  for (NsVector::iterator nsi = Nsset.getNsset().begin(); nsi != Nsset.getNsset().end(); nsi++)
  {
    (*nsi)->print();    
  }
  printf("---------------------------------\n");
  printf("Keys:\n");
  for (iter = daos.begin(); iter != daos.end(); iter++)
  {
    bool sig = false;
    RRList_t keys = (*iter)->getKeyset();
    for (RRIter_t kit = keys.begin(); kit != keys.end(); kit++)
    {
      printf(" * ");
      ((DnsDnskey*)(*kit))->print();
      sig = true;
    }
    RRList_t sigs = (*iter)->getSigset();
    if (sig)
    {
      for(RRIter_t sit = sigs.begin(); sit != sigs.end(); sit++)
      {
        printf("==RRSIG== ");
        ((DnsRrsig*)(*sit))->print();
      }
    }
  }
  printf("---------------------------------\n");
  printf("DS:\n");
  for (iter = daos.begin(); iter != daos.end(); iter++)
  {
    bool sig = false;
    RRList_t dss = (*iter)->getDsset();
    for (RRIter_t kit = dss.begin(); kit != dss.end(); kit++)
    {
      printf(" * ");
      ((DnsDs*)(*kit))->print();
      sig = true;
    }
    RRList_t sigs = (*iter)->getSigset();
    if (sig)
    {
      for(RRIter_t sit = sigs.begin(); sit != sigs.end(); sit++)
      {
        printf("==RRSIG==");
        ((DnsRrsig*)(*sit))->print();
      }
    }
  }
  printf("================================\n");

}

bool DsyncZoneDao::deleteSelf()
{
  connect(__FILE__, __LINE__);
  prepareQuery("select ZoneID from DSYNC_ZONE where ZoneName = ?");
  setStr(0, zoneName);
  exec();
  next();
  int iZoneid = getInt(0);
  clearQuery();

  prepareQuery("Select RRSetID from DSYNC_RRSET where ZoneFK = ?");
  setInt(0, iZoneid);
  exec();
  next();
  int iRrsetfk = getInt(0);
  clearQuery();

  //history
  prepareQuery("delete from DSYNC_HISTORY where rrsetfk = ?");
  setInt(0, iRrsetfk);
  update();
  clearQuery();
  
  //rrsig
  prepareQuery("delete from DSYNC_RRSIG where rrsetfk = ?");
  setInt(0, iRrsetfk);
  update();
  clearQuery();
  
  //dnskey
  prepareQuery("delete from DSYNC_DNSKEY where rrsetfk = ?");
  setInt(0, iRrsetfk);
  update();
  clearQuery();
  
  //ds
  prepareQuery("delete from DSYNC_DS where rrsetfk = ?");
  setInt(0, iRrsetfk);
  update();
  clearQuery();
  
  //rrset
  prepareQuery("delete from DSYNC_RRSET where rrsetid = ?");
  setInt(0, iRrsetfk);
  update();
  clearQuery();
  
  //ns
  prepareQuery("delete from DSYNC_NAMESERVER where zonefk = ?");
  setInt(0, iZoneid);
  update();
  clearQuery();  

  //zone
  prepareQuery("delete from DSYNC_ZONE where zoneid = ?");
  setInt(0, iZoneid);
  update();
  clearQuery();
  
  ps_elog(PSL_DEBUG, "Delete all instances of zone %s from database\n", zoneName.c_str());
  disconnect();

  return true;
}

uint32_t DsyncZoneDao::getDnskeyTTL()
{
  std::vector<DsyncRrsetDao*>::iterator iter;
  std::vector<DsyncRrsetDao*> daos = Rrsets;
  uint32_t ttl = 0;
  for (iter = daos.begin(); iter != daos.end(); iter++)
  {
    RRList_t keys = (*iter)->getKeyset();
    for (RRIter_t kit = keys.begin(); kit != keys.end(); kit++)
    {
      if (((DnsDnskey*)(*kit))->ttl() > ttl)
      {
        ttl = ((DnsDnskey*)(*kit))->ttl();
      }
    }
  }
  return ttl;
}

uint32_t DsyncZoneDao::getDsTTL()
{
  std::vector<DsyncRrsetDao*>::iterator iter;
  std::vector<DsyncRrsetDao*> daos = Rrsets;
  uint32_t ttl = 0;
  for (iter = daos.begin(); iter != daos.end(); iter++)
  {
    RRList_t keys = (*iter)->getDsset();
    for (RRIter_t kit = keys.begin(); kit != keys.end(); kit++)
    {
      if (((DnsDs*)(*kit))->ttl() > ttl)
      {
        ttl = ((DnsDs*)(*kit))->ttl();
      }
    }
  }
  return ttl;
}

void DsyncZoneDao::clearList(RRList_t &p_oList)
{
  for (RRIter_t tIter = p_oList.begin();
       p_oList.end() != tIter;
       tIter++)
  {
    	delete *tIter;
  }
  p_oList.clear();
}
