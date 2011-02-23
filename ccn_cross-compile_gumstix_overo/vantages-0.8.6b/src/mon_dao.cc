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

#include "mon_dao.h"
#include "ps_defs.h"

using namespace std;

const char *MonitorListDao::s_kszDaoName = "MONITOR_LIST_DAO";

const char *MonitorListDao::s_szSelect = "select m.SRC \
  from PS_MONITOR_LIST m \
  where NEXT_POLL < ? \
  order by m.SRC;";

const char *MonitorListDao::s_szSelectNextTime = "select VALUE \
  from PS_CONFIG \
  where NAME = 'POLL_PERIOD';";

const char *MonitorListDao::s_kszUpdateList = "update PS_MONITOR_LIST set LAST_POLL = ?, NEXT_POLL = ?;";

MonitorListDao::MonitorListDao()
  : m_iPollPeriod(0)
{

}

MonitorListDao::~MonitorListDao()
{

}

bool MonitorListDao::serialize()
{
  bool bRet = false;

  time_t tNow = time(NULL);
  if (!connect(__FILE__, __LINE__))
  {
    ps_elog(PSL_CRITICAL, "Unable to connect.\n");
  }
  else if (!prepareQuery(s_kszUpdateList))
  {
    ps_elog(PSL_CRITICAL, "Unable to prepare query: '%s'\n", s_kszUpdateList);
  }
  else if (!setInt(0, tNow))
  {
    ps_elog(PSL_CRITICAL, "Unable to set time in list update.\n");
  }
  else if (!setInt(1, tNow + m_iPollPeriod))
  {
    ps_elog(PSL_CRITICAL, "Unable to set time in scheduler update.\n");
  }
  else if (!update())
  {
    ps_elog(PSL_CRITICAL, "Unable to update scheduler.\n");
  }
  else
  {
    bRet = true;
  }
  disconnect();

  return bRet;
}

bool MonitorListDao::deserialize(DaoList_t &p_oOutputList)
{
  bool bRet = false;

  time_t tNow = time(NULL);
  string sPeriod;
  if (!clear())
  {
    ps_elog(PSL_CRITICAL, "Unable to clear.\n");
  }
  else if (!connect(__FILE__, __LINE__))
  {
    ps_elog(PSL_CRITICAL, "Unable to connect.\n");
  }
  else if (!prepareQuery(s_szSelectNextTime))
  {
    ps_elog(PSL_CRITICAL, "Unable to prepare query: '%s'\n", s_szSelectNextTime);
  }
  else if (!exec())
  {
    ps_elog(PSL_CRITICAL, "Unable to exec().\n");
  }
  else if (!next())
  {
    ps_elog(PSL_CRITICAL, "No configuration value found for polling period.\n");
  }
  else if (!getStr(0, sPeriod))
  {
    ps_elog(PSL_CRITICAL, "Unable to get period from query.\n");
  }
  else
  {
    m_iPollPeriod = (int) strtol(sPeriod.c_str(), (char **)NULL, 10);
    disconnect();
    if (!connect(__FILE__, __LINE__))
    {
      ps_elog(PSL_CRITICAL, "Unable to connect to DB.\n");
    }
    else if (!prepareQuery(s_szSelect))
    {
      ps_elog(PSL_CRITICAL, "Unable to prepare query: '%s'\n", s_szSelect);
    }
    else if (!setInt(0, tNow))
    {
      ps_elog(PSL_CRITICAL, "Unable to set current time.\n");
    }
    else if (!exec())
    {
      ps_elog(PSL_CRITICAL, "Unable to exec().\n");
    }
    else
    {
      while (next())
      {
        string sSrc;
        if (!getStr(0, sSrc))
        {
          ps_elog(PSL_CRITICAL, "Unable to getStr on index 0\n");
        }
        else
        {
          m_tList.push_back(sSrc);
        }
      }
      bRet = true;
    }
  }
  disconnect();

  return bRet;
}

MonitorListDao *MonitorListDao::dup()
{
  return new MonitorListDao();
}

std::string MonitorListDao::daoName()
{
  return string(s_kszDaoName);
}

UrlIter_t MonitorListDao::begin()
{
  return m_tList.begin();
}

UrlIter_t MonitorListDao::end()
{
  return m_tList.end();
}

size_t MonitorListDao::size()
{
  return m_tList.size();
}

bool MonitorListDao::empty()
{
  return m_tList.empty();
}

bool MonitorListDao::clear()
{
  m_tList.clear();

  return true;
}

