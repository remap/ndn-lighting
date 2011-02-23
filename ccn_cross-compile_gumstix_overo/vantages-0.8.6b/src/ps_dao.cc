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
#include <string.h>
#include <sqlite3.h>
//#include <sqlite3ext.h>

#include "config.h"

#include "ps_dao.h"
#include "ps_config.h"
#include "ps_mutex_hdlr.h"
#include "ps_logger.h"
#include "ps_defs.h"

using namespace std;

int PsDao::s_iMaxCxns = 4;
string PsDao::s_sConnectString;
list<sqlite3 *> PsDao::s_oCxnList;
list<sqlite3 *> PsDao::s_oFreeCxnList;
PsMutex PsDao::s_oMutex;

#ifdef _PS_DEBUG
static map<int, string> m_oWhereMap;
#endif

PsDao::PsDao()
  : m_iID(-1),
    m_iNsID(-1),
    m_pCxn(NULL),
    m_pStmt(NULL),
    m_szError(NULL),
    m_bConnected(false)
{

}

PsDao::~PsDao()
{
  disconnect();
}

int PsDao::getID()
{
  return m_iID;
}

void PsDao::setID(int p_iID)
{
  m_iID = p_iID;
}

int PsDao::getNsID()
{
  return m_iNsID;
}

void PsDao::setNsID(int p_iNsID)
{
  m_iNsID = p_iNsID;
}

std::string &PsDao::getSig()
{
  return m_sSig;
}

bool PsDao::setSig(std::string &p_sSig)
{
  m_sSig = p_sSig;

  return true;
}

const char *PsDao::getError()
{
  return (const char *) (m_szError) ? m_szError : "NO ERROR";
}

std::string PsDao::getConnectString()
{
  return s_sConnectString;
}

void PsDao::setConnectString(std::string &p_sConnectString)
{
  s_sConnectString = p_sConnectString;
}


bool PsDao::connect(const char *p_szFile, int p_iLine)
{
  bool bRet = true;

#ifdef _PS_DEBUG
  char sz[11];
  memset(sz, 0, 11);
  sprintf(sz, "%d", p_iLine);
#endif

  disconnect();

  //
  // CRITICAL SECTION BEGIN
  //
  while (!m_bConnected)
  {
    PsMutexHandler oHndlr(s_oMutex);

    if (s_sConnectString.length() == 0)
    {
      const char *szCxn = PsConfig::getInstance().getValue(PS_CONFIG_KEY_DB_CXN_STRING);
      if (NULL == szCxn)
      {
        ps_elog(PSL_CRITICAL, "No DB cxn string defined\n");
      }
      else
      {
        s_sConnectString = szCxn;
      }
    }

#ifdef _PS_DEBUG
    string sKey = p_szFile;
    sKey += ":";
    sKey += sz;
#endif

    if (!s_oFreeCxnList.empty())
    {
#ifdef _PS_DEBUG
      ps_elog(PSL_CRITICAL, "REUSING %s\n", sKey.c_str());
      m_oWhereMap[(int) m_pCxn] = sKey;
#endif
      m_pCxn = s_oFreeCxnList.front();
      s_oFreeCxnList.pop_front();
      m_bConnected = true;
    }
    else if (s_oCxnList.size() < (unsigned) s_iMaxCxns)
    {
#ifdef _PS_DEBUG
      ps_elog(PSL_CRITICAL, "MAKING %s\n", sKey.c_str());
#endif
      sqlite3 *pCxn = NULL;
      int iErr = sqlite3_open(s_sConnectString.c_str(), &pCxn);
      if (0 != iErr)
      {
        m_szError = (char *) sqlite3_errmsg(pCxn);
        ps_elog(PSL_CRITICAL, "Unable to open DB: '%s'\n", m_szError);
      }
      else
      {
        m_pCxn = pCxn;
        m_bConnected = true;
        iErr = sqlite3_busy_timeout(m_pCxn, 15000);
        if (0 != iErr)
        {
          m_szError = (char *) sqlite3_errmsg(pCxn);
          ps_elog(PSL_CRITICAL, "Unable to set busy handler for new connection... continuing...: %s\n", m_szError);
        }
        prepareQuery("PRAGMA synchronous = OFF;");
        update();
        clearQuery();
        s_oCxnList.push_back(pCxn);
        bRet = m_bConnected;
#ifdef _PS_DEBUG
        m_oWhereMap[(int) m_pCxn] = sKey;
#endif
      }
    }
    else
    {
#ifdef _PS_DEBUG
      ps_elog(PSL_CRITICAL, "WQINTING... %s\n", p_szFile);
      for (map<int, string>::iterator tIter = m_oWhereMap.begin();
          m_oWhereMap.end() != tIter;
          tIter++)
      {
        ps_elog(PSL_CRITICAL, "\t%s\n", tIter->second.c_str());
      }

#endif
      s_oMutex.wait(2000);
    }
  }
  //
  // CRITICAL SECTION END
  //

  return bRet;
}

bool PsDao::disconnect()
{
  bool bRet = false;

  clearQuery();

  //
  // CRITICAL SECTION BEGIN
  //
  if (NULL != m_pCxn)
  {
    PsMutexHandler oHndlr(s_oMutex);

#ifdef _PS_DEBUG
    if (m_oWhereMap.end() != m_oWhereMap.find((int) m_pCxn))
    {
      m_oWhereMap.erase(m_oWhereMap.find((int) m_pCxn));
    }
    ps_elog(PSL_CRITICAL, "CHECKED IN\n");
#endif
    s_oFreeCxnList.push_back(m_pCxn);
    m_pCxn = NULL;
    m_bConnected = false;
    bRet = true;


    s_oMutex.signal();
  }
  //
  // CRITICAL SECTION END
  //

  m_sQuery = "";

  m_szError = NULL;

  return bRet;
}

bool PsDao::isConnected()
{
  return m_bConnected;
}

bool PsDao::clearQuery()
{
  bool bRet = true;

  if (NULL != m_pStmt)
  {
    sqlite3_finalize(m_pStmt);
    m_pStmt = NULL;
  }

  return bRet;
}

bool PsDao::finalize()
{
  while (!s_oCxnList.empty())
  {
    sqlite3 *pCxn = s_oCxnList.front();
    sqlite3_close(pCxn);
    s_oCxnList.pop_front();
  }

  return true;
}

bool PsDao::prepareQuery(std::string &p_sQuery)
{
  bool bRet = false;

  if (0 == p_sQuery.length())
  {
    ps_elog(PSL_CRITICAL, "Unable to prepare empty query.\n");
  }
  else
  {
    bRet = prepareQuery(p_sQuery.c_str());
  }

  return bRet;
}

bool PsDao::prepareQuery(const char *p_szQuery)
{
  bool bRet = false;

if (NULL != m_pStmt)
{
  sqlite3_finalize(m_pStmt);
  m_pStmt = NULL;
}


  if (NULL == p_szQuery)
  {
    ps_elog(PSL_CRITICAL, "Unable to prepare NULL query.\n");
  }
  else if (!m_bConnected)
  {
    ps_elog(PSL_CRITICAL, "DB not connected.\n");
  }
//  else if (SQLITE_OK != sqlite3_prepare(m_pCxn, p_sQuery.c_str(), -1, &m_pStmt, NULL))
  else if (SQLITE_OK != sqlite3_prepare_v2(m_pCxn, p_szQuery, -1, &m_pStmt, NULL))
  {
    m_szError = (char *) sqlite3_errmsg(m_pCxn);
    ps_elog(PSL_CRITICAL, "Unable to prepare statement: '%s' because: '%s'\n",
           p_szQuery,
           m_szError);
  }
  else
  {
    m_sQuery = p_szQuery;
    bRet = true;
  }

  return bRet;
}

bool PsDao::setInt(int p_iIndex, int p_iVal)
{
  bool bRet = false;

  int iErr = 0;
  if (!m_bConnected)
  {
    ps_elog(PSL_CRITICAL, "Unable to set parameter before connecting to DB.\n");
  }
  else if (NULL == m_pStmt)
  {
    ps_elog(PSL_CRITICAL, "Unable to set parameter before calling prepareQuery().\n");
  }
  else if (SQLITE_OK != (iErr = sqlite3_bind_int(m_pStmt, p_iIndex + 1, p_iVal)))
  {
    m_szError = (char *) sqlite3_errmsg(m_pCxn);
    ps_elog(PSL_CRITICAL, "Unable to execute bind because of: %d\n", iErr);
  }
  else
  {
    bRet = true;
  }

  return bRet;
}

bool PsDao::setDouble(int p_iIndex, double p_dVal)
{
  bool bRet = false;

  int iErr = 0;
  if (!m_bConnected)
  {
    ps_elog(PSL_CRITICAL, "Unable to set parameter before connecting to DB.\n");
  }
  else if (NULL == m_pStmt)
  {
    ps_elog(PSL_CRITICAL, "Unable to set parameter before calling prepareQuery().\n");
  }
  else if (SQLITE_OK != (iErr = sqlite3_bind_double(m_pStmt, p_iIndex + 1, p_dVal)))
  {
    m_szError = (char *) sqlite3_errmsg(m_pCxn);
    ps_elog(PSL_CRITICAL, "Unable to execute bind because of: %d\n", iErr);
  }
  else
  {
    bRet = true;
  }

  return bRet;
}

bool PsDao::setStr(int p_iIndex, std::string &p_sVal)
{
  bool bRet = false;

  int iErr = 0;
  if (!m_bConnected)
  {
    ps_elog(PSL_CRITICAL, "Unable to set parameter before connecting to DB.\n");
  }
  else if (NULL == m_pStmt)
  {
    ps_elog(PSL_CRITICAL, "Unable to set parameter before calling prepareQuery().\n");
  }
  else if (SQLITE_OK != (iErr = sqlite3_bind_text(m_pStmt, p_iIndex + 1, p_sVal.c_str(), p_sVal.length(), SQLITE_TRANSIENT)))
  {
    m_szError = (char *) sqlite3_errmsg(m_pCxn);
    ps_elog(PSL_CRITICAL, "Unable to execute bind because of: [%d] %s\n", iErr, m_szError);
  }
  else
  {
    bRet = true;
  }

  return bRet;
}

bool PsDao::setBlob(int p_iIndex, char *p_pBuff, size_t p_uLen)
{
  bool bRet = false;

  int iErr = 0;
  if (!m_bConnected)
  {
    ps_elog(PSL_CRITICAL, "Unable to set parameter before connecting to DB.\n");
  }
  else if (NULL == m_pStmt)
  {
    ps_elog(PSL_CRITICAL, "Unable to set parameter before calling prepareQuery().\n");
  }
  else if (SQLITE_OK != (iErr = sqlite3_bind_blob(m_pStmt, p_iIndex + 1, p_pBuff, p_uLen, SQLITE_TRANSIENT)))
  {
    m_szError = (char *) sqlite3_errmsg(m_pCxn);
    ps_elog(PSL_CRITICAL, "Unable to execute bind because of: %d\n", iErr);
  }
  else
  {
    bRet = true;
  }

  return bRet;
}

bool PsDao::setNULL(int p_iIndex)
{
  bool bRet = false;

  int iErr = 0;
  if (!m_bConnected)
  {
    ps_elog(PSL_CRITICAL, "Unable to set parameter before connecting to DB.\n");
  }
  else if (NULL == m_pStmt)
  {
    ps_elog(PSL_CRITICAL, "Unable to set parameter before calling prepareQuery().\n");
  }
  else if (SQLITE_OK != (iErr = sqlite3_bind_null(m_pStmt, p_iIndex + 1)))
  {
    m_szError = (char *) sqlite3_errmsg(m_pCxn);
    ps_elog(PSL_CRITICAL, "Unable to execute bind because of: %d\n", iErr);
  }
  else
  {
    bRet = true;
  }

  return bRet;
}

bool PsDao::exec()
{
  bool bRet = false;

  if (!m_bConnected)
  {
    ps_elog(PSL_CRITICAL, "DB not connected.\n");
  }
  else if (NULL == m_pStmt)
  {
    ps_elog(PSL_CRITICAL, "Statement not prepared, call prepareQuery() before exec().\n");
  }
/*
//  else if (SQLITE_OK != sqlite3_prepare(m_pCxn, p_sQuery.c_str(), -1, &m_pStmt, NULL))
  else if (SQLITE_OK != sqlite3_prepare_v2(m_pCxn, p_sQuery.c_str(), -1, &m_pStmt, NULL))
  {
    m_szError = (char *) sqlite3_errmsg(m_pCxn);
    ps_elog(PSL_CRITICAL, "Unable to prepare statement: '%s' because: '%s'\n",
           p_sQuery.c_str(),
           m_szError);
  }
*/
  else
  {
    bRet = true;
  }

  return bRet;
}

bool PsDao::update()
{
  bool bRet = false;

  int iErr = 0;
  if (!m_bConnected)
  {
    ps_elog(PSL_CRITICAL, "DB not connected.\n");
  }
  else if (NULL == m_pStmt)
  {
    ps_elog(PSL_CRITICAL, "Statement not active.\n");
  }
  else if (SQLITE_DONE == (iErr = sqlite3_step(m_pStmt)))
  {
    bRet = true;
  }
  else
  {
    m_szError = (char *) sqlite3_errmsg(m_pCxn);
    ps_elog(PSL_WARNING, "Error while stepping: [%d] '%s'\n", iErr, m_szError);
  }

  return bRet;
}

bool PsDao::next()
{
  bool bRet = false;

  int iErr = 0;
  if (!m_bConnected)
  {
    ps_elog(PSL_CRITICAL, "DB not connected.\n");
  }
  else if (NULL == m_pStmt)
  {
    ps_elog(PSL_CRITICAL, "Statement not active.\n");
  }
  else if (SQLITE_DONE == (iErr = sqlite3_step(m_pStmt)))
  {
    bRet = false;
  }
  else if (SQLITE_ROW != iErr)
  {
    m_szError = (char *) sqlite3_errmsg(m_pCxn);
    ps_elog(PSL_CRITICAL, "Error while stepping: [%d] '%s'\n", iErr, m_szError);
  }
  else
  {
    bRet = true;
  }

  return bRet;
}

int PsDao::getInt(int p_iCol)
{
  int iRet = 0;

  int iType = 0;
  if (!m_bConnected)
  {
    ps_elog(PSL_CRITICAL, "DB not connected.\n");
  }
  else if (NULL == m_pStmt)
  {
    ps_elog(PSL_CRITICAL, "Statement not active.\n");
  }
  else if (SQLITE_INTEGER != (iType = sqlite3_column_type(m_pStmt, p_iCol)))// + 1)))
  {
    ps_elog(PSL_CRITICAL, "Column %d is %d (not integer).\n", p_iCol, iType);
  }
  else
  {
    iRet = sqlite3_column_int(m_pStmt, p_iCol);
  }

  return iRet;
}

double PsDao::getDouble(int p_iCol)
{
  double dRet = 0.0;

  int iType = 0;
  if (!m_bConnected)
  {
    ps_elog(PSL_CRITICAL, "DB not connected.\n");
  }
  else if (NULL == m_pStmt)
  {
    ps_elog(PSL_CRITICAL, "Statement not active.\n");
  }
  else if (SQLITE_FLOAT != (iType = sqlite3_column_type(m_pStmt, p_iCol)))// + 1)))
  {
    ps_elog(PSL_CRITICAL, "Column is %d (not integer).\n", iType);
  }
  else
  {
    dRet = sqlite3_column_double(m_pStmt, p_iCol);
  }

  return dRet;
}

bool PsDao::getStr(int p_iCol, std::string &p_sOutput)
{
  bool bRet = false;

  if (!m_bConnected)
  {
    ps_elog(PSL_CRITICAL, "DB not connected.\n");
  }
  else if (NULL == m_pStmt)
  {
    ps_elog(PSL_CRITICAL, "Statement not active.\n");
  }
  else
  {
    const char *szCol = (const char *) sqlite3_column_text(m_pStmt, p_iCol);// + 1);
    int iLen = sqlite3_column_bytes(m_pStmt, p_iCol);// + 1);
    p_sOutput.assign(szCol, iLen);
    bRet = true;
  }

  return bRet;
}

bool PsDao::getBlob(int p_iCol, char **p_ppOutput, int &p_iOutLen)
{
  bool bRet = false;

  if (!m_bConnected)
  {
    ps_elog(PSL_CRITICAL, "DB not connected.\n");
  }
  else if (NULL == m_pStmt)
  {
    ps_elog(PSL_CRITICAL, "Statement not active.\n");
  }
  else if (NULL == p_ppOutput)
  {
    ps_elog(PSL_CRITICAL, "Unable to fetch column with NULL output variable.\n");
  }
  else
  {
    const char *pCol = (const char *) sqlite3_column_blob(m_pStmt, p_iCol);// + 1);
    p_iOutLen = sqlite3_column_bytes(m_pStmt, p_iCol);// + 1);
    *p_ppOutput = new char[p_iOutLen];
    memcpy(*p_ppOutput, pCol, p_iOutLen);
    bRet = true;
  }

  return bRet;
}

bool PsDao::isNULL(int p_iCol)
{
  bool bRet = false;

  int iType = -1;
  if (!m_bConnected)
  {
    ps_elog(PSL_CRITICAL, "DB not connected.\n");
  }
  else if (NULL == m_pStmt)
  {
    ps_elog(PSL_CRITICAL, "Statement not active.\n");
  }
  else if (SQLITE_NULL != (iType = sqlite3_column_type(m_pStmt, p_iCol)))// + 1)))
  {
    ps_elog(PSL_DEBUG, "Column %d is %d (not NULL).\n", p_iCol, iType);
  }
  else
  {
    bRet = true;
  }

  return bRet;
}

int PsDao::getLastID()
{
  int iRet = -1;

  if (!m_bConnected)
  {
    ps_elog(PSL_CRITICAL, "DB not connected.\n");
  }
  else
  {
    iRet = (int) sqlite3_last_insert_rowid(m_pCxn);
  }

  return iRet;
}

void PsDao::clearList(DaoList_t &p_oList)
{
  for (DaoIter_t tIter = p_oList.begin();
       p_oList.end() != tIter;
       tIter++)
  {
    delete *tIter;
  }

  p_oList.clear();
}

