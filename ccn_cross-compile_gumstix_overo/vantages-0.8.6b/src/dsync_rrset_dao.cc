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

#include "dsync_rrset_dao.h"
#include "dsync_zone_dao.h"
#include "ps_defs.h"
#include "dao_factory.h"
#include "raw_data_dao.h"
#include <time.h>
#include "dns_name.h"

using namespace std;

const char* DsyncRrsetDao::selectKeyset = "SELECT  ZoneName, Protocol, Algorithm, TTL, b64Key, RData \
                                           FROM DSYNC_DNSKEY d inner join DSYNC_RRSET r on d.RRSetFK = r.RRSetID \
                                           inner join DSYNC_ZONE z on z.ZoneID = r.ZoneFK \
                                           where ZoneName = '?'";

const char* DsyncRrsetDao::insertKey = "INSERT into DSYNC_DNSKEY (PROTOCOL, ALGORITHM, TTL, B64KEY, RDATA, RRSETFK) \
                                        VALUES (?, ?, ?, ?, ?, ?)";

const char* DsyncRrsetDao::deleteRRSet = "DELETE from DSYNC_DNSKEY where RRSetFK = ?; \
                                          DELETE from DSYNC_DS where RRSetFK = ?; \
                                          DELETE from DSYNC_RRSET where RRSetID = ?";


DsyncRrsetDao::DsyncRrsetDao()// : v_oDs(NULL), v_oDnskey(NULL), v_oSig(NULL)
{
  //string cs = "/home3/tuchsche/dsync.db";
  //setConnectString(cs);
  nsName = "";
  nsIp = "";
}

DsyncRrsetDao::~DsyncRrsetDao()
{
  DsyncZoneDao::clearList(v_oDnskey);
  DsyncZoneDao::clearList(v_oDs);
  DsyncZoneDao::clearList(v_oSig);
}

int DsyncRrsetDao::getDatabaseID()
{
  return iDatabaseID;
}

void DsyncRrsetDao::setDatabaseID(int i)
{
  iDatabaseID = i;
}


bool DsyncRrsetDao::serializeDnskey()
{
  //If Dnskey set matches DB, extend the RRSig expire time
  //  If not, add a new RRSet

  DsyncZoneDao* zoneDao = new DsyncZoneDao();
  zoneDao->zoneName = zoneName;
  zoneDao->deserialize();
  bool bExists = false;
  for (unsigned int i = 0; i < zoneDao->getRrsets().size(); i++)
  {
    DsyncRrsetDao *tDao = zoneDao->getRrsets().at(i);
    //if our set matches tDao it's already in the DB, update LastSeen 
    if (setsMatch(tDao->getKeyset(), getKeyset()))
    {
      setDatabaseID(tDao->getDatabaseID());
      bExists = true;
    }
  }
  delete zoneDao;

  if (!connect(__FILE__, __LINE__))
  {
    ps_elog(PSL_ERROR, "Unable to connect: '%s'\n", getError());
  }

  //if already in the db, update the one we have
  if (bExists)
  {
    clearQuery();
    if (nsName == "" && nsIp == "")
    {
      if (!prepareQuery("UPDATE DSYNC_RRSET SET LastSeen = ? where RRSetID = ?"))
      {
        ps_elog(PSL_ERROR, "Unable to prepare query: '%s'\n", getError());
      }
      else if (!setInt(0, time(0)))
      {
        ps_elog(PSL_ERROR, "Unable to set LastSeen: '%s'\n", getError());
      }
      else if (!setInt(1, iDatabaseID))
      {
        ps_elog(PSL_ERROR, "Unable to set RRSetID: '%s'\n", getError());
      }
      else if (!update())
      {
        ps_elog(PSL_ERROR, "Unable to update RRSet: '%s'\n", getError());
      }
      clearQuery();
    }
    else
    {
      if (!prepareQuery("Select NameserverID from DSYNC_NAMESERVER where NameserverName = ? and NameserverIP = ?"))
      {
        ps_elog(PSL_ERROR, "Unable to prepareQuery: '%s'\n", getError());
      }
      else if (!setStr(0, nsName))
      {
        ps_elog(PSL_ERROR, "Unable to set nameserver name: '%s'\n", getError());
      }
      else if (!setStr(1, nsIp))
      {
        ps_elog(PSL_ERROR, "Unable to set nameserver IP: '%s'\n", getError());
      }
      else if (!exec())
      {
        ps_elog(PSL_ERROR, "Unable to execute query: '%s'\n", getError());
      }
      else if (!next())
      {
        ps_elog(PSL_ERROR, "No results, Nameserver is not in DB: '%s'\n", getError());
      }
      int NameserverFK = getInt(0);
      clearQuery();

      if (!prepareQuery("UPDATE DSYNC_RRSET SET LastSeen = ?, NameserverFK = ? where RRSetID = ?"))
      {
        ps_elog(PSL_ERROR, "Unable to prepare query: '%s'\n", getError());
      }
      else if (!setInt(0, time(0)))
      {
        ps_elog(PSL_ERROR, "Unable to set LastSeen: '%s'\n", getError());
      }
      else if (!setInt(1,NameserverFK))
      {
        ps_elog(PSL_ERROR, "Unable to set NameserverFK: '%s'\n", getError());
      }
      else if (!setInt(2, iDatabaseID))
      {
        ps_elog(PSL_ERROR, "Unable to set RRSetID: '%s'\n", getError());
      }
      else if (!update())
      {
        ps_elog(PSL_ERROR, "Unable to update RRSet: '%s'\n", getError());
      }
      clearQuery();
    }
  }
  else
  {
    // If not already in the db, add a new RRSet
    prepareQuery("SELECT ZoneID from DSYNC_ZONE WHERE ZoneName = ?");
    setStr(0, zoneName);
    exec();
    next();
    int zId = getInt(0);
    if (next())
    {
      printf("Got back more than one matching zone...\n");
    }
    clearQuery();
    if (nsName == "" && nsIp == "")
    {
      if(!prepareQuery("INSERT into DSYNC_RRSET (ZoneFK, LastSeen) VALUES (?, ?)"))
      {
        ps_elog(PSL_ERROR, "Error preparing query: '%s'\n", getError());
      }
      else if (!setInt(0, zId))
      {
        ps_elog(PSL_ERROR, "Failed to set ZoneFK: '%s'\n", getError());
      }
      else if (!setInt(1, time(0)))
      {
        ps_elog(PSL_ERROR, "Failed to set LastSeen: '%s'\n", getError());
      } 
      else if (!update())
      {
        ps_elog(PSL_ERROR, "Unable to update: '%s'\n", getError());
      }
    }
    else
    {
      if (!prepareQuery("Select NameserverID from DSYNC_NAMESERVER where NameserverName = ? and NameserverIP = ?"))
      {
        ps_elog(PSL_ERROR, "Unable to prepareQuery: '%s'\n", getError());
      }
      else if (!setStr(0, nsName))
      {
        ps_elog(PSL_ERROR, "Unable to set nameserver name: '%s'\n", getError());
      }
      else if (!setStr(1, nsIp))
      {
        ps_elog(PSL_ERROR, "Unable to set nameserver IP: '%s'\n", getError());
      }
      else if (!exec())
      {
        ps_elog(PSL_ERROR, "Unable to execute query: '%s'\n", getError());
      }
      else if (!next())
      {
        ps_elog(PSL_ERROR, "No results, Nameserver is not in DB: '%s'\n", getError());
      }
      int NameserverFK = getInt(0);
      clearQuery();
      if (!prepareQuery("INSERT into DSYNC_RRSET (ZoneFK, LastSeen, NameserverFK) VALUES (?, ?, ?)"))
      {
        ps_elog(PSL_ERROR, "Error prepareing query: '%s'\n", getError());
      }
      else if (!setInt(0, zId))
      {
        ps_elog(PSL_ERROR, "Failed to set ZoneFK: '%s'\n", getError());
      }
      else if (!setInt(1, time(0)))
      {
        ps_elog(PSL_ERROR, "Failed to set LastSeen: '%s'\n", getError());
      } 
      else if (!setInt(2, NameserverFK))
      {
        ps_elog(PSL_ERROR, "Failed to set NameserverFK: '%s'\n", getError());
      }
      else if (!update())
      {
        ps_elog(PSL_ERROR, "Unable to update: '%s'\n", getError());
      }

    }
    int rrId = getLastID();
    iDatabaseID = rrId;
    clearQuery();

    for (RRIter_t iter = v_oDnskey.begin();
        iter != v_oDnskey.end();
        iter++)
    {
      DnsDnskey key = *(DnsDnskey*) *iter;
      if (!prepareQuery(insertKey))
      {
        ps_elog(PSL_ERROR, "Unable to prepare query: '%s'\n", getError());
      }
      //add each key
      else if (!setInt(0, key.getProto()))
      {
        ps_elog(PSL_ERROR, "Unable to set protocol: '%s'\n", getError());
      }
      else if (!setInt(1, key.getAlgo()))  //algorithm
      {
        ps_elog(PSL_ERROR, "Unable to set Algorithm: '%s'\n", getError());
      }
      else if (!setInt(2, key.ttl()))  //ttl
      {
        ps_elog(PSL_ERROR, "Unable to set TTL: '%s'\n", getError());
      }
      else if (!setStr(3, key.getKey())) //b64key
      {
        ps_elog(PSL_ERROR, "Unable to set Key: '%s'\n", getError());
      }
      else if (!setBlob(4, (char*)key.get_rdata(), key.get_rdlen()))  //rdata
      {
        ps_elog(PSL_ERROR, "Unable to set rdata: '%s'\n", getError());
      }
      else if (!setInt(5, rrId)) //rrsetfk 
      {
        ps_elog(PSL_ERROR, "Unable to set RRSetFK: '%s'\n", getError());
      }
      else if (!update())
      {
        ps_elog(PSL_ERROR, "Unable to insert Dnskey: '%s'\n", getError());
      }
      clearQuery();
    }
  }
  disconnect();
  return true;
}

bool DsyncRrsetDao::serializeDs()
{
  //If Dnskey set matches DB, extend the RRSig expire time
  //  If not, add a new RRSet

  DsyncZoneDao* zoneDao = new DsyncZoneDao();
  zoneDao->zoneName = zoneName;
  zoneDao->deserialize();
  bool bExists = false;
  //printf("Checking to see if any deserialized zones match me\n");
  for (unsigned int i = 0; i < zoneDao->getRrsets().size(); i++)
  {
    DsyncRrsetDao *tDao = zoneDao->getRrsets().at(i);
    //if our set matches tDao it's already in the DB, update LastSeen 
    if (getDsset().size() == 0 || tDao->getDsset().size() == 0)
    {
      continue;
    }
    else if (setsMatch(tDao->getDsset(), getDsset()))
    {
      setDatabaseID(tDao->getDatabaseID());
      bExists = true;
    }
  }
  delete zoneDao; 

  if (!connect(__FILE__, __LINE__))
  {
    ps_elog(PSL_ERROR, "Unable to connect: '%s'\n", getError());
  }

  if (bExists)
  {
    clearQuery();
    if (nsName == "" && nsIp == "")
    {
      if (!prepareQuery("UPDATE DSYNC_RRSET SET LastSeen = ? where RRSetID = ?"))
      {
        ps_elog(PSL_ERROR, "Unable to prepare query: '%s'\n", getError());
      }
      else if (!setInt(0, time(0)))
      {
        ps_elog(PSL_ERROR, "Unable to set LastSeen: '%s'\n", getError());
      }
      else if (!setInt(1, iDatabaseID))
      {
        ps_elog(PSL_ERROR, "Unable to set RRSetID: '%s'\n", getError());
      }
      else if (!update())
      {
        ps_elog(PSL_ERROR, "Unable to update RRSet: '%s'\n", getError());
      }
      clearQuery();
    }
    else
    {
      if (!prepareQuery("Select NameserverID from DSYNC_NAMESERVER where NameserverName = ? and NameserverIP = ?"))
      {
        ps_elog(PSL_ERROR, "Unable to prepareQuery: '%s'\n", getError());
      }
      else if (!setStr(0, nsName))
      {
        ps_elog(PSL_ERROR, "Unable to set nameserver name: '%s'\n", getError());
      }
      else if (!setStr(1, nsIp))
      {
        ps_elog(PSL_ERROR, "Unable to set nameserver IP: '%s'\n", getError());
      }
      else if (!exec())
      {
        ps_elog(PSL_ERROR, "Unable to execute query: '%s'\n", getError());
      }
      else if (!next())
      {
        ps_elog(PSL_ERROR, "No results, Nameserver is not in DB: '%s'\n", getError());
      }
      int NameserverFK = getInt(0);
      clearQuery();

      if (!prepareQuery("UPDATE DSYNC_RRSET SET LastSeen = ?, NameserverFK = ? where RRSetID = ?"))
      {
        ps_elog(PSL_ERROR, "Unable to prepare query: '%s'\n", getError());
      }
      else if (!setInt(0, time(0)))
      {
        ps_elog(PSL_ERROR, "Unable to set LastSeen: '%s'\n", getError());
      }
      else if (!setInt(1,NameserverFK))
      {
        ps_elog(PSL_ERROR, "Unable to set NameserverFK: '%s'\n", getError());
      }
      else if (!setInt(2, iDatabaseID))
      {
        ps_elog(PSL_ERROR, "Unable to set RRSetID: '%s'\n", getError());
      }
      else if (!update())
      {
        ps_elog(PSL_ERROR, "Unable to update RRSet: '%s'\n", getError());
      }
      clearQuery();
    }
  }
  else
  {
    // If not already in the db, add a new RRSet
    prepareQuery("SELECT ZoneID from DSYNC_ZONE WHERE ZoneName = ?");
    setStr(0, zoneName);
    exec();
    next();
    int zId = getInt(0);
    if (next())
    {
      printf("Got back more than one matching zone...\n");
    }
    clearQuery();
    if (nsName == "" && nsIp == "")
    {
      if(!prepareQuery("INSERT into DSYNC_RRSET (ZoneFK, LastSeen) VALUES (?, ?)"))
      {
        ps_elog(PSL_ERROR, "Error preparing query: '%s'\n", getError());
      }
      else if (!setInt(0, zId))
      {
        ps_elog(PSL_ERROR, "Failed to set ZoneFK: '%s'\n", getError());
      }
      else if (!setInt(1, time(0)))
      {
        ps_elog(PSL_ERROR, "Failed to set LastSeen: '%s'\n", getError());
      } 
      else if (!update())
      {
        ps_elog(PSL_ERROR, "Unable to update: '%s'\n", getError());
      }
    }
    else
    {
      int NameserverFK = -1;

      if (!prepareQuery("Select NameserverID from DSYNC_NAMESERVER where NameserverName = ? and NameserverIP = ?"))
      {
        ps_elog(PSL_ERROR, "Unable to prepareQuery: '%s'\n", getError());
      }
      else if (!setStr(0, nsName))
      {
        ps_elog(PSL_ERROR, "Unable to set nameserver name: '%s'\n", getError());
      }
      else if (!setStr(1, nsIp))
      {
        ps_elog(PSL_ERROR, "Unable to set nameserver IP: '%s'\n", getError());
      }
      else if (!exec())
      {
        ps_elog(PSL_ERROR, "Unable to execute query: '%s'\n", getError());
      }
      else if (!next())
      {
        ps_elog(PSL_ERROR, "No results, Nameserver is not in DB: '%s'\n", getError());
      }
      else
      {
        NameserverFK = getInt(0);
      }
      clearQuery();

      if (!prepareQuery("INSERT into DSYNC_RRSET (ZoneFK, LastSeen, NameserverFK) VALUES (?, ?, ?)"))
      {
        ps_elog(PSL_ERROR, "Error prepareing query: '%s'\n", getError());
      }
      else if (!setInt(0, zId))
      {
        ps_elog(PSL_ERROR, "Failed to set ZoneFK: '%s'\n", getError());
      }
      else if (!setInt(1, time(0)))
      {
        ps_elog(PSL_ERROR, "Failed to set LastSeen: '%s'\n", getError());
      } 
      else if (!setInt(2, NameserverFK))
      {
        ps_elog(PSL_ERROR, "Failed to set NameserverFK: '%s'\n", getError());
      }
      else if (!update())
      {
        ps_elog(PSL_ERROR, "Unable to update: '%s'\n", getError());
      }

    }
    int rrId = getLastID();
    iDatabaseID = rrId;
    clearQuery();



    //----
    // ds

    //printf("serializing ds... size=%d\n", (int)v_oDs.size());    

    for (RRIter_t iter = v_oDs.begin();
        iter != v_oDs.end();
        iter++)
    {
      DnsDs *ds = (DnsDs*) *iter;

      //ds->print();

      if (!prepareQuery("insert into DSYNC_DS (Protocol, Algorithm, TTL, B16Digest, RData, RRSetFK) values (?, ?, ?, ?, ?, ?)"))
      {
        ps_elog(PSL_ERROR, "Unable to prepareQuery: '%s'\n");
      }
      else if (!setInt(0, ds->getProto()))
      {
        ps_elog(PSL_ERROR, "Unable to set protocol: '%s'\n");
      }
      else if (!setInt(1, ds->getAlgo()))
      {
        ps_elog(PSL_ERROR, "Unable to set algorithm: '%s'\n");
      }
      else if (!setInt(2, ds->ttl()))
      {
        ps_elog(PSL_ERROR, "Unable to set ttl: '%s'\n");
      }
      else if (!setStr(3, ds->getDig()))
      {
        ps_elog(PSL_ERROR, "Unable to set digest: '%s'\n");
      }
      else if (!setBlob(4, (char*)ds->get_rdata(), ds->get_rdlen()))
      {
        ps_elog(PSL_ERROR, "Unable to set rdata: '%s'\n");
      }
      else if (!setInt(5, rrId))
      {
        ps_elog(PSL_ERROR, "Unable to set RRSetFK: '%s'\n");
      }
      else if (!update())
      {
        ps_elog(PSL_ERROR, "Unable to insert DS: '%s'\n");
      }
      clearQuery();
    }

  }
  disconnect();
  return true;
}

bool DsyncRrsetDao::serializeRrsig()
{
  if (!connect(__FILE__, __LINE__))
  {
    ps_elog(PSL_ERROR, "Unable to connect: '%s'\n", getError());
  }
  
  //get the rrsig set that covers zone with iDatabaseID
  //for each rrsig in my signature set
  //  if the signature is in the db set
  //    extend expire time for the one in the db set
  //    dbsigid = the id
  //  else
  //    add it to the database
  //    dbsigid = last id
  //  add an entry to history table (dbsigid, iDatabaseID, rdata)

  if (!prepareQuery("Select RRSigID, RData from DSYNC_RRSIG where RRSetFK = ?"))
  {
    ps_elog(PSL_ERROR, "Unable to prepare query: '%s'\n", getError());
  }
  else if (!setInt(0, iDatabaseID))
  {
    ps_elog(PSL_ERROR, "Unable to set DatabaseID: '%s'\n", getError());
  }
  else if (!exec())
  {
    ps_elog(PSL_ERROR, "Unable to execute query: '%s'\n", getError());
  }

  std::vector<int> sigIds;
  std::vector<DnsRrsig*> sigs;

  while (next())
  {
    int rdlen = 0;
    char* rdata = NULL;
    sigIds.push_back(getInt(0));
    DnsRrsig* sig = new DnsRrsig();
    getBlob(1, &rdata, rdlen);
    sig->set_rdata((u_char*)rdata, rdlen);
    sig->parseRData(NULL, 0, (u_char*)rdata, rdlen);
    sigs.push_back(sig);

    if (rdata)
    {
      delete [] rdata;
    }
  }
  clearQuery(); 

  for (RRIter_t iter = v_oSig.begin();
       iter != v_oSig.end();
       iter++)
  {
    int HRRSigFK = -1;
    bool sawSig = false;
    for (unsigned int i = 0; i < sigs.size(); i++)
    {
      if ((*(DnsRrsig*)(*iter)) == *sigs.at(i))
      {
        //printf("Old sig\n");
        HRRSigFK = sigIds.at(i);
        if (!prepareQuery("update DSYNC_RRSIG set LastSeen = ? where RRSigID = ?"))
        {
          ps_elog(PSL_ERROR, "Unable to prepare query: '%s'\n", getError());
        }
        else if (!setInt(0, time(0)))
        {
          ps_elog(PSL_ERROR, "Unable to set LastSeen: '%s'\n", getError());
        }
        else if (!setInt(1, HRRSigFK))
        {
          ps_elog(PSL_ERROR, "Unable to set RRSigID: '%s'\n", getError());
        }
        else if (!update())
        {
          ps_elog(PSL_ERROR, "Unable to update RRSig: '%s'\n", getError());
        }
        clearQuery();
        sawSig = true;
        break;
      }
    }

    if (!sawSig)
    {
      DnsRrsig s = *(DnsRrsig*) (*iter);
      if (!prepareQuery("insert into DSYNC_RRSIG (LastSeen, RRSetFK, RData) values (?, ?, ?)"))
      {
        ps_elog(PSL_ERROR, "Unable to prepare query: '%s'\n", getError());
      }
      else if (!setInt(0, time(0)))
      {
        ps_elog(PSL_ERROR, "Unable to set LastSeen: '%s'\n", getError());
      }
      else if (!setInt(1, iDatabaseID))
      {
        ps_elog(PSL_ERROR, "Unable to set RRSetFK: '%s'\n", getError());
      }
      else if (!setBlob(2,  (char*)s.get_rdata(), s.get_rdlen()))
      {
        ps_elog(PSL_ERROR, "Unable to set RData: '%s'\n", getError());
      }
      else if (!update())
      {
        ps_elog(PSL_ERROR, "Unable to insert RRSig: '%s'\n", getError());
      }
      clearQuery();
      HRRSigFK = getLastID();
    }
    if (nsName == "" && nsIp == "")
    {
      if (!prepareQuery("insert into DSYNC_HISTORY (Timestamp, RRSigFK, RRSetFK) values (?, ?, ?)"))
      {
        ps_elog(PSL_ERROR, "Unable to prepare query: '%s'\n", getError());
      }
      else if (!setInt(0, time(0)))
      {
        ps_elog(PSL_ERROR, "Unable to set timestamp: '%s'\n", getError());
      }
      else if (!setInt(1, HRRSigFK))
      {
        ps_elog(PSL_ERROR, "Unable to set RRSigFK: '%s'\n", getError());
      }
      else if (!setInt(2, iDatabaseID))
      {
        ps_elog(PSL_ERROR, "Unable to set RRSetFK: '%s'\n", getError());
      }
      else if (!update())
      {
        ps_elog(PSL_ERROR, "Unable to insert to history: '%s'\n", getError());
      }
      clearQuery();
    }
    else
    {
      if (!prepareQuery("insert into DSYNC_HISTORY (Timestamp, RRSigFK, RRSetFK, NameserverIP) values (?, ?, ?, ?)"))
      {
        ps_elog(PSL_ERROR, "Unable to prepare query: '%s'\n", getError());
      }
      else if (!setInt(0, time(0)))
      {
        ps_elog(PSL_ERROR, "Unable to set timestamp: '%s'\n", getError());
      }
      else if (!setInt(1, HRRSigFK))
      {
        ps_elog(PSL_ERROR, "Unable to set RRSigFK: '%s'\n", getError());
      }
      else if (!setInt(2, iDatabaseID))
      {
        ps_elog(PSL_ERROR, "Unable to set RRSetFK: '%s'\n", getError());
      }
      else if (!setStr(3, nsIp))
      {
        ps_elog(PSL_ERROR, "Unable to set NsIP: '%s'\n", getError());
      }
      else if (!update())
      {
        ps_elog(PSL_ERROR, "Unable to insert to history: '%s'\n", getError());
      }
      clearQuery();
    }
  } 
  std::vector<DnsRrsig*>::iterator sit;
  for (sit = sigs.begin(); sit != sigs.end(); sit++)
  {
    delete *sit;
  }
  disconnect();
  return true;
}

bool DsyncRrsetDao::serialize()
{
    if (!serializeDnskey())
    {
      ps_elog(PSL_DEBUG, "serializeDnskey() returned false\n");
      return false;
    }
    if (!serializeDs())
    {
      ps_elog(PSL_DEBUG, "serializeDs() returned false\n");
      return false;
    }
    if (!serializeRrsig())
    {
      ps_elog(PSL_DEBUG, "serializERrsig() returned false\n");
      return false;
    }
    return true;
 }

bool DsyncRrsetDao::deserialize()
{
  if (getDatabaseID() == -1)
  {
    printf("Deserialize called on an rr with databaseid = -1... returning\n");
    return false;
  }
  bool bRet = false;
  
  if (!connect(__FILE__, __LINE__))
  {
    ps_elog(PSL_ERROR, "Unable to connect: '%s'\n", getError());
  }
  
  //read all the keys attached to this rrset
  prepareQuery("select Protocol, Algorithm, TTL, B64Key, RData from DSYNC_DNSKEY where RRSetFK = ?");
  if (!setInt(0, iDatabaseID))
    printf("Unable to set RRSetFK in the query\n");
  if (!exec())
    printf("Unable to exec()");
  while (next())
  {
    int rDataLen = 0;
    char* rData = NULL;
    DnsName oName(zoneName);
    DnsDnskey *oKey = new DnsDnskey();
    oKey->set_name(oName);
    oKey->setProto(getInt(0));
    oKey->setAlgo(getInt(1));
    oKey->set_ttl(getInt(2));
    std::string tKey;
    getStr(3, tKey);
    oKey->setKey(tKey);
    getBlob(4, &rData, rDataLen);
    oKey->set_rdata((u_char*)rData, rDataLen);
    v_oDnskey.push_back(oKey);
    
    //printf("Deserialized key: \n");
    oKey->parseRData(NULL, 0, (u_char*)rData, rDataLen);
    //oKey->print();
    if (rData)
    {
      delete [] rData;
    }
  }
  clearQuery();

  //read all the DSs attached to this rrset
  prepareQuery("select Protocol, Algorithm, TTL, B16Digest, RData from DSYNC_DS where RRSetFK = ?");
  //printf("Query DB - getting DSs for RRSET = %d\n", iDatabaseID);
  setInt(0, iDatabaseID);
  exec();
  while (next())
  {
    int rDataLen = 0;
    char* rData = NULL;
    DnsName oName(zoneName);
    DnsDs* oDs = new DnsDs();
    oDs->set_name(oName);
    oDs->setProto(getInt(0));
    oDs->setAlgo(getInt(1));
    oDs->set_ttl(getInt(2));
    std::string *tDig = new std::string();
    getStr(3, *tDig);
    std::string ttDig = *tDig;
    delete tDig;
    oDs->setDig(ttDig);
    getBlob(4, &rData, rDataLen);
    oDs->set_rdata((u_char*)rData, rDataLen);
    oDs->parseRData(NULL, 0, (u_char*)rData, rDataLen);
    v_oDs.push_back(oDs);
   
    if (rData)
    {
      delete [] rData;
    }
  }

  clearQuery();
  
  disconnect();
  bRet = true;
  return bRet;
}

bool DsyncRrsetDao::deserialize(DaoList_t &p_oOutputList)
{
  bool bRet = false;

  return bRet;
}

DsyncRrsetDao *DsyncRrsetDao::dup()
{
  return new DsyncRrsetDao();
}

std::string DsyncRrsetDao::daoName()
{
  return "";
}

void DsyncRrsetDao::addKey(DnsDnskey * key)
{
  v_oDnskey.push_back(key);
  //printf("v_oDnskey.size() = %i\n", (int)v_oDnskey.size());
}

bool DsyncRrsetDao::deleteKey(DnsDnskey * key)
{
  bool bRet = false;
  for (RRIter_t iter = v_oDnskey.begin();
      iter != v_oDnskey.end();
      iter++)
  {
    if ((DnsDnskey*) *iter == key)
    {
      v_oDnskey.erase(iter);
      bRet = true;
    }
  }  
  return bRet;
}

void DsyncRrsetDao::addDs(DnsDs * ds)
{
  v_oDs.push_back(ds);
}

void DsyncRrsetDao::addSig(DnsRrsig * sig)
{
  v_oSig.push_back(sig);
}

bool DsyncRrsetDao::deleteDs(DnsDs *ds)
{
  bool bRet = false;
  for (RRIter_t iter = v_oDs.begin();
      iter != v_oDs.end();
      iter++)
  {
    if ((DnsDs*) *iter == ds)
    {
      v_oDs.erase(iter);
      bRet = true;
    }
  }  
  return bRet;
}

RRList_t &DsyncRrsetDao::getKeyset()
{
  return v_oDnskey;
}

RRList_t &DsyncRrsetDao::getDsset()
{
  return v_oDs;
}

RRList_t &DsyncRrsetDao::getSigset()
{
  return v_oSig;
}

bool DsyncRrsetDao::setKeyset(RRList_t keyset)
{
  bool bRet = false;
  //if the sets are the same, return
  if (setsMatch(v_oDnskey, keyset))
  { 
    //printf("IF...\n");
    bRet = true;
  }
  else
  {
    //printf("ELSE...\n");
    //otherwise, do XXX with the old set

    //point v_oDnskey at the new set
    v_oDnskey = keyset;
    bRet = true;
  }
  return bRet;
}

bool DsyncRrsetDao::setSigset(RRList_t sigset)
{
  //printf("setSigset called.\n");
  /*
  if (setsMatch(v_oSig, sigset))
  {
    return true;
  }
  else
  {
  */
    //printf("setSigset: setting v_oSig\n");
    v_oSig = sigset;
  //}

  /*
  //printf("setSigset: printing list:\n");
  for (RRIter_t it = v_oSig.begin();
      it != v_oSig.end();
      it++)
  {
    printf("**(%d)**: ", (int)(*it)->get_rdlen());
    ((DnsRrsig*)(*it))->print();
  }
  */
  return true;
}

bool DsyncRrsetDao::setDsset(RRList_t DsSet)
{
  //printf("DsyncRrsetDao::setDsset called on with the following set:\n");
  //for (RRIter_t i = DsSet.begin(); i != DsSet.end(); i++)
  //{
  //  (*i)->print();
  //}

  if (v_oDs == DsSet)
  {
    return true;
  }
  else
  {
    v_oDs = DsSet;
  }
  return true;
}

bool DsyncRrsetDao::setsMatch(RRList_t v1, RRList_t v2)
{
  bool bEqual = true;
  if (v1.size() != v2.size())
    bEqual = false;
  for (RRIter_t iter = v1.begin();
      iter != v1.end();
      iter++)
  {
    if (bEqual == false)
      break;
    bool found = false;
    for (RRIter_t i2 = v2.begin();
        i2 != v2.end();
        i2++)
    {
      if(*(DnsDnskey*)*iter == *(DnsDnskey*)*i2)
      {
        found = true;
        break;   
      }
    }
    if (!found)
      bEqual = false;
  }
  return bEqual;
}

void DsyncRrsetDao::setSourceNs(std::string n, std::string i)
{
  nsName = n;
  nsIp = i;
}
