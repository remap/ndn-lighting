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
#include <stdio.h>

#include <iostream>
#include <sstream>

#include "ps_local_key_dao.h"
#include "ps_logger.h"

using namespace std;

const char *PsLocalKeyDao::s_kszDaoName = "PS_LOCAL_KEY_DAO";

const char *PsLocalKeyDao::s_szSelectKey = "select ID, NAME, PUB_KEY, PRIV_KEY, PHRASE from PS_LOCAL_KEY";

PsLocalKeyDao::PsLocalKeyDao()
{

}

PsLocalKeyDao::~PsLocalKeyDao()
{

}

std::string &PsLocalKeyDao::getKeyName()
{
  return m_sKeyName;
}

std::string &PsLocalKeyDao::getPubKey()
{
  return m_sPub;
}

std::string &PsLocalKeyDao::getPrivKey()
{
  return m_sPriv;
}

std::string &PsLocalKeyDao::getPhrase()
{
  return m_sPP;
}


bool PsLocalKeyDao::serialize()
{
  ps_elog(PSL_CRITICAL, "Not implemented.\n");

  return false;
}

bool PsLocalKeyDao::deserialize()
{
  bool bRet = false;

  int iID = getID();
  ostringstream oSS;
  oSS << s_szSelectKey;
  if (iID > -1)
  {
    oSS << " where ID = " << iID;
  }

  if (!connect(__FILE__, __LINE__))
  {
    ps_elog(PSL_CRITICAL, "Unable to connect to DB: '%s'\n", getError());
  }
  else if (!prepareQuery(oSS.str().c_str()))
  {
    ps_elog(PSL_CRITICAL, "Unable to prepare SQL: '%s' -> '%s'\n", oSS.str().c_str(), getError());
  }
  else if (!exec())
  {
    ps_elog(PSL_CRITICAL, "Unable to exec: '%s'\n", getError());
  }
  else if (!next())
  {
    ps_elog(PSL_CRITICAL, "Unable to get next: '%s'\n", getError());
  }
  else if (!getStr(1, m_sKeyName))
  {
    ps_elog(PSL_CRITICAL, "Unable to get key name: '%s'\n", getError());
  }
  else if (!getStr(2, m_sPub))
  {
    ps_elog(PSL_CRITICAL, "Unable to get public key: '%s'\n", getError());
  }
  else if (!getStr(3, m_sPriv))
  {
    ps_elog(PSL_CRITICAL, "Unable to get priv key: '%s'\n", getError());
  }
  else if (!getStr(4, m_sPP))
  {
    ps_elog(PSL_CRITICAL, "Unable to get phrase: '%s'\n", getError());
  }
  else
  {
    setID(getInt(0));

    if (next())
    {
      ps_elog(PSL_CRITICAL, "Too many results returned.\n");
      setID(-1);
    }
    else
    {
      bRet = true;
    }
  }

  return bRet;
}

bool PsLocalKeyDao::deserialize(DaoList_t &p_oOutputList)
{
  ps_elog(PSL_CRITICAL, "Not implemented.\n");

  return false;
}


std::string PsLocalKeyDao::daoName()
{
  return string(s_kszDaoName);
}

PsLocalKeyDao *PsLocalKeyDao::dup()
{
  return new PsLocalKeyDao();
}
