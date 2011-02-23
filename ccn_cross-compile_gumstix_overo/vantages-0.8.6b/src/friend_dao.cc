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
#include "ps_logger.h"
#include <stdio.h>

#include <sstream>
#include <iostream>

#include "friend_dao.h"
#include "ps_defs.h"
#include "dao_factory.h"
#include "raw_data_dao.h"

using namespace std;

const char *FriendDao::s_kszDaoName = "FRIEND_DAO";
const char *FriendDao::s_szSelectFriends = "select URL, ID, NAME, PASSWD, PUB_KEY, FIRST_SEEN, LAST_SEEN from PS_FRIEND";
const char *FriendDao::s_szInsertFriend = "insert into PS_FRIEND (URL, NAME, PASSWD, PUB_KEY, FIRST_SEEN, LAST_SEEN) \
                                            values (?, ?, ?, ?, ?, ?)";
const char *FriendDao::s_szDeleteFriend = "delete from PS_FRIEND where ID = ?";

FriendDao::FriendDao()
  : m_iFriendID(-1),
    m_bDelete(false),
    m_tFirstSeen(-1),
    m_tLastSeen(-1)
{

}

FriendDao::~FriendDao()
{

}

int FriendDao::getFriendID()
{
  return m_iFriendID;
}

void FriendDao::setFriendID(int p_iID)
{
  m_iFriendID = p_iID;
}

time_t FriendDao::getFirstSeen()
{
  return m_tFirstSeen;
}

void FriendDao::setFirstSeen(time_t p_tSeen)
{
  m_tFirstSeen = p_tSeen;
}

time_t FriendDao::getLastSeen()
{
  return m_tLastSeen;
}

void FriendDao::setLastSeen(time_t p_tSeen)
{
  m_tLastSeen = p_tSeen;
}

std::string &FriendDao::getURL()
{
  return m_sURL;
}

void FriendDao::setURL(std::string &p_sURL)
{
  m_sURL = p_sURL;
}

std::string &FriendDao::getName()
{
  return m_sName;
}

void FriendDao::setName(std::string &p_sName)
{
  m_sName = p_sName;
}

std::string &FriendDao::getPass()
{
  return m_sPass;
}

void FriendDao::setPass(std::string &p_sPass)
{
  m_sPass = p_sPass;
}

std::string &FriendDao::getKey()
{
  return m_sKey;
}

void FriendDao::setKey(std::string &p_sKey)
{
  m_sKey = p_sKey;
}

bool FriendDao::getDelete()
{
  return m_bDelete;
}

void FriendDao::setDelete(bool p_bDelete)
{
  m_bDelete = p_bDelete;
}

bool FriendDao::serialize()
{
  bool bRet = false;

  if (!connect(__FILE__, __LINE__))
  {
    ps_elog(PSL_ERROR, "Unable to connect: '%s'\n", getError());
  }
  else if (getDelete())
  {
    if (!prepareQuery(s_szDeleteFriend))
    {
      ps_elog(PSL_ERROR, "Unable to prepare query '%s': '%s'\n", s_szDeleteFriend, getError());
    }
    else if (!setInt(0, getFriendID()))
    {
      ps_elog(PSL_ERROR, "Unable to set Friend ID: '%s'\n", getError());
    }
    else if (!update())
    {
      ps_elog(PSL_ERROR, "Unable to delete friend: '%s'\n", getError());
    }
    else
    {
      ps_elog(PSL_DEBUG, "Deleted friend.\n");
      bRet = true;
    }
  }
  else
  {
    if (!prepareQuery(s_szInsertFriend))
    {
      ps_elog(PSL_ERROR, "Unable to prepare query '%s': '%s'\n", s_szInsertFriend, getError());
    }
    else if (!setStr(0, getURL()))
    {
      ps_elog(PSL_ERROR, "Unable to set URL '%s': '%s\n", getURL().c_str(), getError());
    }
    else if (!setStr(1, getName()))
    {
      ps_elog(PSL_ERROR, "Unable to set name '%s': '%s'\n", getName().c_str(), getError());
    }
    else if (!setStr(2, getPass()))
    {
      ps_elog(PSL_ERROR, "Unable to set pass <...>: '%s'\n", getError());
    }
    else if (!setStr(3, getKey()))
    {
      ps_elog(PSL_ERROR, "Unable to set key '%s': '%s'\n", getKey().c_str(), getError());
    }
    else if (!setInt(4, getFirstSeen()))
    {
      ps_elog(PSL_ERROR, "Unable to set first seen: '%s'\n", getError());
    }
    else if (!setInt(5, getLastSeen()))
    {
      ps_elog(PSL_ERROR, "Unable to set last seen: '%s'\n", getError());
    }
    else if (!update())
    {
      ps_elog(PSL_ERROR, "Unable to update friend: '%s'\n", getError());
    }
    else
    {
      ps_elog(PSL_DEBUG, "Inserted new friend.\n");
      bRet = true;
    }
  }

  return bRet;
}

bool FriendDao::deserialize()
{
  bool bRet = false;

  ostringstream oSS;
  oSS << s_szSelectFriends << " where ID = ?";
  string sURL;
  string sName;
  string sPubKey;
  string sQuery = oSS.str();

  if (-1 == getFriendID())
  {
    ps_elog(PSL_CRITICAL, "Unable to deserialize DAO w/o FriendID.\n");
  }
  else if (!connect(__FILE__, __LINE__))
  {
    ps_elog(PSL_CRITICAL, "Unable to connect to DB\n");
  }
  else if (!prepareQuery(sQuery))
  {
    ps_elog(PSL_CRITICAL, "Unable to prepareQuery: '%s'\n", sQuery.c_str());
  }
  else if (!setInt(0, getFriendID()))
  {
    ps_elog(PSL_CRITICAL, "Unable to set bind param for friend ID\n");
  }
  else if (!exec())
  {
    ps_elog(PSL_CRITICAL, "Unable to exec() query.\n");
  }
  else if (!next())
  {
    ps_elog(PSL_CRITICAL, "Unable to query for ID: %d\n", getFriendID());
  }
  else if (!getStr(0, sURL))
  {
    ps_elog(PSL_CRITICAL, "Unable to get URL from query.\n");
  }
  else if (!getStr(2, sName))
  {
    ps_elog(PSL_CRITICAL, "Unable to get URL from query.\n");
  }
  else if (!getStr(4, sPubKey))
  {
    ps_elog(PSL_CRITICAL, "Unable to get URL from query.\n");
  }
  else if (next())
  {
    ps_elog(PSL_CRITICAL, "Error: more than one result returned by query for ID: %d\n", getFriendID());
  }
  else
  {
    setName(sName);
    setKey(sPubKey);
    setFirstSeen(getInt(5));
    setLastSeen(getInt(6));
    setURL(sURL);
    bRet = true;
  }
  disconnect();

  return bRet;
}

bool FriendDao::deserialize(DaoList_t &p_oOutputList)
{
  bool bRet = false;

  clearList(p_oOutputList);
  if (!connect(__FILE__, __LINE__))
  {
    ps_elog(PSL_CRITICAL, "Unable to connect()\n");
  }
  else if (!prepareQuery(s_szSelectFriends))
  {
    ps_elog(PSL_CRITICAL, "Unable to prepareQuery() for query: '%s'\n", s_szSelectFriends);
  }
  else if (!exec())
  {
    ps_elog(PSL_CRITICAL, "Unable to exec query: '%s'\n", s_szSelectFriends);
  }
  else
  {
    bRet = true;
    while (next())
    {
      string sURL;
      string sName;
      string sPubKey;
      if (!getStr(0, sURL))
      {
        ps_elog(PSL_CRITICAL, "Unable to get URL.\n");
        bRet = false;
        break;
      }
      else if (!getStr(2, sName))
      {
        ps_elog(PSL_CRITICAL, "Unable to get URL from query.\n");
            bRet = false;
            break;
      }
      else if (!getStr(4, sPubKey))
      {
        ps_elog(PSL_CRITICAL, "Unable to get URL from query.\n");
            bRet = false;
            break;
      }
      else
      {
        FriendDao *pDao = static_cast<FriendDao *>(DaoFactory::getInstance().create(daoName()));
        pDao->setFriendID(getInt(1));
        pDao->setURL(sURL);
        pDao->setName(sName);
        pDao->setKey(sPubKey);
        pDao->setFirstSeen(getInt(5));
        pDao->setLastSeen(getInt(6));
        p_oOutputList.push_back(pDao);
      }
    }
  }
  disconnect();

  if (!bRet)
  {
    clearList(p_oOutputList);
  }

  return bRet;
}

FriendDao *FriendDao::dup()
{
  return new FriendDao();
}

std::string FriendDao::daoName()
{
  return s_kszDaoName;
}

