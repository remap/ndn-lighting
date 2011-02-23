/*
 * Copyright (c) 2008,2009, University of California, Los Angeles and 
    ps_elog(PSL_CRITICAL, "m_sHomeDir = %s\n", m_sHomeDir.c_str());
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

#include <stdio.h>

#include <string>

#include "ps_policy_dao.h"
#include "ps_logger.h"
#include "ps_defs.h"

using namespace std;

const char *PsPolicyDao::s_kszDaoName = "PS_POLICY_DAO";

const char *PsPolicyDao::s_szSelectPolicy = "select RULE, ACTION, POLICY_ORDER from PS_POLICY where TYPE_ID = ? order by POLICY_ORDER asc";
const char *PsPolicyDao::s_szDeletePolicy = "delete from PS_POLICY where TYPE_ID = ?";
const char *PsPolicyDao::s_szInsertPolicy = "insert into PS_POLICY (RULE, ACTION, POLICY_ORDER, TYPE_ID) values (?, ?, ?, ?)";

PsPolicyDao::PsPolicyDao()
  : m_iPolicyType(0)
{

}

PsPolicyDao::~PsPolicyDao()
{
  clear();
}

int PsPolicyDao::getType()
{
  return m_iPolicyType;
}

bool PsPolicyDao::setType(int p_iType)
{
  m_iPolicyType = p_iType;

  return true;
}

bool PsPolicyDao::addDefaultRule(ps_policy_e p_ePolicy)
{
  PolicyType_t tPolicy;
  tPolicy.m_sRule = "";
  tPolicy.m_ePolicy = p_ePolicy;
  m_oPolicyList.push_back(tPolicy);

  return true;
}

bool PsPolicyDao::addRule(char *p_szRule, ps_policy_e p_ePolicy)
{
  bool bRet = false;

  if (NULL == p_szRule)
  {
    ps_elog(PSL_ERROR, "Unable to add NULL rule.\n");
  }
  else if (PS_POLICY_UNKNOWN == p_ePolicy)
  {
    ps_elog(PSL_ERROR, "Unable to add policy with UNKNOWN action.\n");
  }
  else
  {
    string sRule = p_szRule;
    bRet = addRule(sRule, p_ePolicy);
  }

  return bRet;
}

bool PsPolicyDao::addRule(const char *p_szRule, ps_policy_e p_ePolicy)
{
  bool bRet = false;

  if (NULL == p_szRule)
  {
    ps_elog(PSL_ERROR, "Unable to add NULL rule.\n");
  }
  else if (PS_POLICY_UNKNOWN == p_ePolicy)
  {
    ps_elog(PSL_ERROR, "Unable to add policy with UNKNOWN action.\n");
  }
  else
  {
    string sRule = p_szRule;
    bRet = addRule(sRule, p_ePolicy);
  }

  return bRet;
}

bool PsPolicyDao::addRule(std::string &p_sRule, ps_policy_e p_ePolicy)
{
  bool bRet = false;

  PolicyType_t tPolicy;
  tPolicy.m_sRule = p_sRule;
  tPolicy.m_ePolicy = p_ePolicy;

  m_oPolicyList.push_front(tPolicy);
  bRet = true;

  return bRet;
}

PsPolicyDao::PsPolicyIter_t PsPolicyDao::begin()
{
  return m_oPolicyList.begin();
}

PsPolicyDao::PsPolicyIter_t PsPolicyDao::end()
{
  return m_oPolicyList.end();
}

void PsPolicyDao::clear()
{
  m_oPolicyList.clear();
}

ps_policy_e PsPolicyDao::check(char *p_szData)
{
  ps_policy_e eRet = PS_POLICY_UNKNOWN;

  if (NULL == p_szData)
  {
    ps_elog(PSL_ERROR, "Unable to use NULL string to check.\n");
  }
  else
  {
    string sData = p_szData;
    eRet = check(sData);
  }

  return eRet;
}

ps_policy_e PsPolicyDao::check(const char *p_szData)
{
  ps_policy_e eRet = PS_POLICY_UNKNOWN;

  if (NULL == p_szData)
  {
    ps_elog(PSL_ERROR, "Unable to use NULL string to check.\n");
  }
  else
  {
    string sData = p_szData;
    eRet = check(sData);
  }

  return eRet;
}

ps_policy_e PsPolicyDao::check(std::string &p_sData)
{
  ps_policy_e eRet = PS_POLICY_UNKNOWN;

  for (PsPolicyIter_t tIter = begin();
       end() != tIter;
       tIter++)
  {
    PolicyType_t &tPolicy = *tIter;
    if (tPolicy.m_sRule == ""
        || p_sData == tPolicy.m_sRule)
    {
      eRet = tPolicy.m_ePolicy;
      ps_elog(PSL_DEBUG, "Policy found for '%s' matches '%s' -> %d\n", 
              p_sData.c_str(), 
              tPolicy.m_sRule.c_str(), 
              (int) eRet);
      break;
    }
    ps_elog(PSL_DEBUG, "Policy for '%s' does NOT match '%s' -> %d\n", 
              p_sData.c_str(), 
              tPolicy.m_sRule.c_str(), 
              (int) eRet);
  }

  return eRet;
}

void PsPolicyDao::print()
{
  int i = 0;
  for (PsPolicyIter_t tIter = begin();
       end() != tIter;
       tIter++)
  {
    PolicyType_t &tPolicy = *tIter;
    fprintf(stdout, "%d: '%s' -> %d\n", i++, tPolicy.m_sRule.c_str(), (int) tPolicy.m_ePolicy);
  }
}

bool PsPolicyDao::serialize()
{
  bool bRet = false;

  if (!connect(__FILE__, __LINE__))
  {
    ps_elog(PSL_CRITICAL, "Unable to connect to DB: %s\n", getError());
  }
  else if (!prepareQuery(s_szDeletePolicy))
  {
    ps_elog(PSL_CRITICAL, "Unable to prepareQuery(): '%s': %s\n", s_szDeletePolicy, getError());
  }
  else if (!setInt(0, getType()))
  {
    ps_elog(PSL_CRITICAL, "Unable to set int: %s\n", getError());
  }
  else if (!update())
  {
    ps_elog(PSL_CRITICAL, "Unable to delete: %s\n", getError());
  }
  else
  {
    int i = 0;
    for (PsPolicyIter_t tIter = begin();
         end() != tIter;
         tIter++)
    {
      PolicyType_t &tPolicy = *tIter;
      if (!clearQuery())
      {
        ps_elog(PSL_CRITICAL, "Unable to clearQuery(): %s\n", getError());
      }
      else if (!prepareQuery(s_szInsertPolicy))
      {
        ps_elog(PSL_CRITICAL, "Unable to prepareQuery(): '%s': %s\n", s_szInsertPolicy, getError());
      }
      else if (!setStr(0, tPolicy.m_sRule))
      {
        ps_elog(PSL_CRITICAL, "Unable to setStr() '%s': %s\n", tPolicy.m_sRule.c_str(), getError());
      }
      else if (!setInt(1, (int) tPolicy.m_ePolicy))
      {
        ps_elog(PSL_CRITICAL, "Unable to setInt() %d: %s\n", (int) tPolicy.m_ePolicy, getError());
      }
      else if (!setInt(2, i++))
      {
        ps_elog(PSL_CRITICAL, "Unable to setInt() %d: %s\n", (i - 1), getError());
      }
      else if (!setInt(3, getType()))
      {
        ps_elog(PSL_CRITICAL, "Unable to setInt() %d: %s\n", getType(), getError());
      }
      else if (!update())
      {
        ps_elog(PSL_CRITICAL, "Unable to update: %s\n", getError());
      }
      else
      {
        ps_elog(PSL_DEBUG, "Added policy row.\n");
        bRet = true;
      }
    }

    disconnect();
  }

  return bRet;
}

bool PsPolicyDao::deserialize()
{
  bool bRet = false;

  clear();

  if (!connect(__FILE__, __LINE__))
  {
    ps_elog(PSL_CRITICAL, "Unable to connect to DB: %s\n", getError());
  }
  else if (!prepareQuery(s_szSelectPolicy))
  {
    ps_elog(PSL_CRITICAL, "Unable to prepareQuery(): '%s': %s\n", s_szSelectPolicy, getError());
  }
  else if (!setInt(0, getType()))
  {
    ps_elog(PSL_CRITICAL, "Unable to setInt() %d: %s\n", getType(), getError());
  }
  else if (!exec())
  {
    ps_elog(PSL_CRITICAL, "Unable to exec() query: %s\n", getError());
  }
  else
  {
    while (next())
    {
      bRet = true;

      string sRule;
      ps_policy_e ePolicy = PS_POLICY_UNKNOWN;
      if (!getStr(0, sRule))
      {
        ps_elog(PSL_CRITICAL, "Unable to setStr() '%s': %s\n", sRule.c_str(), getError());
      }
      else
      {
        ePolicy = (ps_policy_e) getInt(1);
        if (sRule != "" && !addRule(sRule, ePolicy))
        {
          ps_elog(PSL_ERROR, "Unable to addRule() '%s' -> %d\n", sRule.c_str(), (int) ePolicy);
          bRet = false;
          break;
        }
        else if (sRule == "" && !addDefaultRule(ePolicy))
        {
          ps_elog(PSL_ERROR, "Unable to addDefaulRule() '%s' -> %d\n", sRule.c_str(), (int) ePolicy);
          bRet = false;
          break;
        }
      }
    }

    disconnect();
  }

  return bRet;
}

bool PsPolicyDao::deserialize(DaoList_t &p_oOutputList)
{
  bool bRet = false;

  ps_elog(PSL_ERROR, "Not implemented\n");

  return bRet;
}

std::string PsPolicyDao::daoName()
{
  return string(s_kszDaoName);
}

PsPolicyDao *PsPolicyDao::dup()
{
  return new PsPolicyDao();
}

