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

#include "dsync_ns_dao.h"
#include "ps_defs.h"
#include "dao_factory.h"
#include "raw_data_dao.h"

using namespace std;

DsyncNsDao::DsyncNsDao()
{
  //Nss = *(new NsVector());
  Nss.clear();
  //string cs = "/home3/tuchsche/dsync.db";
  //setConnectString(cs);
  //string cs = PsConfig::getInstance().getValue(DSYNC_SQLITE_FILENAME);
  //setConnectString(cs);
}

DsyncNsDao::~DsyncNsDao()
{
  for (NsIter it = Nss.begin(); it != Nss.end(); ++it)
  {
      delete *it;
  }
  //delete Nss;
}

bool DsyncNsDao::serialize()
{
  //if it already exists, do nothing
  DsyncNsDao tDao;
  tDao.zoneName = zoneName;
  tDao.deserialize();
  tDao.disconnect();
  
  if (!connect(__FILE__, __LINE__))
  {
    ps_elog(PSL_ERROR, "Unable to connect: '%s'\n", getError());
  }
  
  for (unsigned int i = 0; i < Nss.size(); i++)
  {
    bool foundMatch = false;
    for (unsigned int k = 0; k < tDao.getNsset().size(); k++)
    {
      if (tDao.getNsset().at(k)->name == Nss.at(i)->name &&
          tDao.getNsset().at(k)->ip == Nss.at(i)->ip)
      {
        foundMatch = true;
        //if our verified status changes, update it
        if (tDao.getNsset().at(k)->verified != Nss.at(i)->verified)
        {
          printf("Verified field didn't match\n");
          if (!setInt(0, (int)Nss.at(i)->verified))
          {
            ps_elog(PSL_ERROR, "Unable to set verified in query: '%s'\n", getError());
          }
          else if (!prepareQuery("Update DSYNC_NAMESERVER set verified = ? where NameserverName = ? and NameserverIp = ?"))
          {
            ps_elog(PSL_ERROR, "Unable to prepare query to update Verified in Nameserver: '%s'\n", getError());
          } 
          else if (!setStr(1, Nss.at(i)->name))
          {
            ps_elog(PSL_ERROR, "Unable to set name in query: '%s'\n", getError());
          } 
          else if (!setStr(2, Nss.at(i)->ip))
          {
            ps_elog(PSL_ERROR, "Unable to set ip in query: '%s'\n", getError()); 
          }
          else if (!update())
          {
            ps_elog(PSL_ERROR, "Unable to execute update: '%s'\n", getError());
          }
          clearQuery();
        }
      }
    }
    //otherwise, insert it
    if (!foundMatch)
    {
      //find our ZoneFK
      if (!prepareQuery("Select ZoneID from DSYNC_ZONE where ZoneName = ?"))
      {
        ps_elog(PSL_ERROR, "Unable to prepare query for ZoneID: '%s'\n", getError());
      }
      else if (!setStr(0, zoneName))
      {
        ps_elog(PSL_ERROR, "Unable to set zoneName in query: '%s'\n", getError());
      }
      else if (!exec())
      {
        ps_elog(PSL_ERROR, "Unable to execute query: '%s'\n", getError());
      }
      else if (!next())
      {
        ps_elog(PSL_ERROR, "Expected results but got none: '%s'\n", getError());
      }
      int zId = getInt(0);
      clearQuery();

      //printf("Preparing to insert to db: (%s, %s, %d, %d)\n", Nss.at(i).name.c_str(), Nss.at(i).ip.c_str(), zId, (int)Nss.at(i).verified);
      //use it
      if (!prepareQuery("insert into DSYNC_NAMESERVER (NameServerName, NameServerIP, ZoneFK, verified) VALUES (?, ?, ?, ?)"))
      {
        ps_elog(PSL_ERROR, "Unable to prepareQuery: '%s'\n", getError());
      }
      else if (!setStr(0, Nss.at(i)->name))
      {
        ps_elog(PSL_ERROR, "Unable to set name for query: '%s'\n", getError());
      }
      else if (!setStr(1, Nss.at(i)->ip))
      {
        ps_elog(PSL_ERROR, "Unable to set IP for query: '%s'\n", getError());
      }
      else if (!setInt(2, zId))
      {
        ps_elog(PSL_ERROR, "Unable to set ZoneFK for query: '%s'\n", getError());
      }
      else if (!setInt(3, (int)Nss.at(i)->verified))
      {
        ps_elog(PSL_ERROR, "Unable to set 'verified' for query: '%s'\n", getError());
      }
      else if (!update())
      {
        ps_elog(PSL_ERROR, "Unable to insert: '%s'\n", getError());
      }
      clearQuery();
    }
  }
  //printf("DsyncNsDao::serialize() returning\n"); 
  disconnect();
  return true;
}

bool DsyncNsDao::deserialize()
{
  if (!connect(__FILE__, __LINE__))
  {
    ps_elog(PSL_ERROR, "Unable to connect: '%s'\n", getError());
  }

  //printf("Select query = SELECT ZoneID from Zone WHERE ZoneName = %s\n", zoneName.c_str()); 
  if (!prepareQuery("SELECT ZoneID from DSYNC_ZONE WHERE ZoneName = ?"))
  {
    ps_elog(PSL_ERROR, "Unable to prepare query: '%s'\n", getError());
  }
  else if (!setStr(0, zoneName))
  {
    ps_elog(PSL_ERROR, "Unable to set zoneName: '%s'\n", getError());
  }
  else if (!exec())
  {
    ps_elog(PSL_ERROR, "Unable to execute query: '%s'\n", getError());
  }
  else if (!next())
  {
    ps_elog(PSL_ERROR, "Expected results but got none: '%s'\n", getError());
  }
  //printf("----\n");
  int zId = getInt(0);
  //printf("----\n");
  clearQuery();
  
  if (!prepareQuery("select NameServerName, NameServerIP, verified from DSYNC_NAMESERVER where ZoneFK = ?"))
  {
    ps_elog(PSL_ERROR, "Unable to prepareQuery: '%s'\n", getError());
  }
  else if (!setInt(0, zId))
  {
    ps_elog(PSL_ERROR, "Unable to set ZoneName: '%s'\n", getError());
  }
  else if (!exec())
  {
    ps_elog(PSL_ERROR, "Unable to execute query: '%s'\n", getError());
  }
  while (next())
  {
    //printf(".\n");
    NsObj* nsp = new NsObj();
    //std::string name;
    //std::string ip;
    getStr(0, nsp->name);
    getStr(1, nsp->ip);
    nsp->verified = getInt(2);
    Nss.push_back(nsp);
  }

  clearQuery();
  disconnect();
  return false;
}

bool DsyncNsDao::deserialize(DaoList_t &p_oOutputList)
{
  bool bRet = false;

  return bRet;
}

DsyncNsDao *DsyncNsDao::dup()
{
  return new DsyncNsDao();
}

std::string DsyncNsDao::daoName()
{
  return "";
}

NsVector & DsyncNsDao::getNsset()
{
  return Nss; 
}

void DsyncNsDao::addNs(NsObj *nsp)
{
  //printf("AddNs called: ");
  //nsp.print();
  Nss.push_back(nsp);
}

void NsObj::print()
{
  printf("%s:  %s (verified = %d)\n", name.c_str(), ip.c_str(), verified);
}

NsObj::NsObj() : verified(false)
{
}

NsObj::~NsObj()
{
}

NsObj* DsyncNsDao::getRandomNs()
{
  int size = Nss.size();
  if (size == 0)
  {
    return NULL;
  }
  else
  {
    int index = rand() % size;
    //printf("Returning the following random nameserver:\n");
    //(Nss.at(index))->print();
    return (Nss.at(index));
  }
  
}
