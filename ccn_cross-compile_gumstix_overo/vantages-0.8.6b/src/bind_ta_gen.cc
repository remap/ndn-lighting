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
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <iostream>
#include <fstream>
#include <sstream>

#include "bind_ta_gen.h"
#include "dnskey_consist_dao.h"
#include "ps_dao.h"
#include "dao_factory.h"
#include "ps_config.h"
#include "ps_strtok.h"
#include "ps_policy_dao.h"
#include "dnskey_defs.h"
#include "ps_logger.h"

using namespace std;

BindTaGen::BindTaGen()
  : m_tStart(0)
{

}

BindTaGen::~BindTaGen()
{
  PsDao::clearList(m_oList);
}

time_t BindTaGen::getStart()
{
  return m_tStart;
}

void BindTaGen::setStart(time_t p_tStart)
{
  m_tStart = p_tStart;
}

bool BindTaGen::genFile(std::string &p_sFile)
{
  bool bRet = false;

  PsDao::clearList(m_oList);
  DnskeyConsistencyDao *pDao = (DnskeyConsistencyDao *) DaoFactory::getInstance().create(DnskeyConsistencyDao::s_kszDaoName);
  PsPolicyDao *pPolicyDao = static_cast<PsPolicyDao *>(DaoFactory::getInstance().create(PsPolicyDao::s_kszDaoName));
  if (NULL == pDao)
  {
    ps_elog(PSL_CRITICAL, "Unable to create DnskeyConsistencyDao\n");
  }
  else if (NULL == pPolicyDao)
  {
    ps_elog(PSL_CRITICAL, "Unable to create PsPolicyDao\n");
  }
  else if (!pDao->deserialize(m_oList))
  {
    ps_elog(PSL_CRITICAL, "Unable to deserialize DAOs.\n");
  }
  else if (m_oList.empty())
  {
    bRet = true;
  }
  else if (!pPolicyDao->setType(DNSKEY_TA_FILE_POLICY_ID) || !pPolicyDao->deserialize())
  {
    ps_elog(PSL_CRITICAL, "Unable to deserialize policy for TA file.\n");
  }
  else
  {
    bool bGen = false;

    // Set the oldest a key can be (i.e. how long since we polled it) before
    // We stop using it.
    int iOldest = 0;
    PsConfig &oConf = PsConfig::getInstance();
    const char *szMaxTaAge = oConf.getValue(PS_CONFIG_TA_MAX_AGE);
    if (NULL == szMaxTaAge)
    {
      iOldest = time(NULL) - 86400;
    }
    else
    {
      iOldest = time(NULL) - (int) strtol(szMaxTaAge, NULL, 10);
    }

    for (DaoIter_t tIter = m_oList.begin();
         m_oList.end() != tIter;
         tIter++)
    {
      DnskeyConsistencyDao *pDao = (DnskeyConsistencyDao *)(*tIter);
      if (pDao->getDate() >= iOldest)
      {
        bGen = true;
        break;
      }
    }

    if (bGen)
    {
      ofstream oFile(p_sFile.c_str());
      if (!oFile.is_open())
      {
        ps_elog(PSL_CRITICAL, "Unable to open: '%s' because: %s\n", p_sFile.c_str(), strerror(errno));
      }
      else
      {
        oFile << "trusted-keys {\n";
        for (DaoIter_t tIter = m_oList.begin();
           m_oList.end() != tIter;
           tIter++)
        {
          DnskeyConsistencyDao *pDao = (DnskeyConsistencyDao *)(*tIter);
          if (pDao->getDate() >= iOldest)
          {
            string sLine;
            string sData = pDao->getData();
            size_t uPos = sData.find("|");
            if (string::npos == uPos)
            {
              ps_elog(PSL_CRITICAL, "Error, cannot parse consistency DAO: '%s'\n", sData.c_str());
            }
            else
            {
              string sCode = sData.substr(0, uPos);
              int iCode = (int) strtol(sCode.c_str(), NULL, 10);

//              if (iCode != PS_F_RULE_CONFLICT)
              if (iCode == PS_F_RULE_CONFIRMED)
              {
                istringstream oISS(sData.substr(uPos + 1));
                while (getline(oISS, sLine) && sLine.length() > 0)
                {
                  PsStrtok oTok(sLine, " ");

                  if (!oTok.hasNext())
                  {
                    ps_elog(PSL_ERROR, "Token does not have flags in line: '%s'\n", sLine.c_str());
                    break;
                  }
                  string sFlags = oTok.next();

                  if (!oTok.hasNext())
                  {
                    ps_elog(PSL_ERROR, "Token does not have proto in line: '%s'\n", sLine.c_str());
                    break;
                  }
                  string sProto = oTok.next();

                  if (!oTok.hasNext())
                  {
                    ps_elog(PSL_ERROR, "Token does not have algo in line: '%s'\n", sLine.c_str());
                    break;
                  }
                  string sAlgo = oTok.next();

                  if (!oTok.hasNext())
                  {
                    ps_elog(PSL_ERROR, "Token does not have key in line: '%s'\n", sLine.c_str());
                    break;
                  }
                  string sKey = "\"" + oTok.next() + "\"";

                  if (PS_POLICY_ALLOW == pPolicyDao->check(sAlgo))
                  {
                    oFile << "\t\"" 
                          << pDao->getName() 
                          << "\"\t" 
                          << sFlags 
                          << "\t" 
                          << sProto 
                          << "\t" 
                          << sAlgo 
                          << "\t" 
                          << sKey 
                          << ";\n";
                  }
/*
                  while (oTok.hasNext())
                  {
                    string sNext = oTok.next();
                    if (!oTok.hasNext())
                    {
                      sNext = "\"" + sNext + "\"";
                    }
                    oFile << " " << sNext;
                  }
                  oFile << ";\n";
*/
                }
              }
            }
          }
        }
        oFile << "};" << endl;
        bRet = true;

        oFile.close();
      }
    }
  }

  if (NULL != pDao)
  {
    delete pDao;
    pDao = NULL;
  }

  if (NULL != pPolicyDao)
  {
    delete pPolicyDao;
    pPolicyDao = NULL;
  }

  return bRet;
}

