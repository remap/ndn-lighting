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
#include <algorithm>

#include "dnskey_set_dao.h"
#include "dns_name.h"
#include "dns_dnskey.h"
#include "dns_rrsig.h"
#include "ps_logger.h"

using namespace std;

const char *DnskeySetDao::s_kszDaoName = "DNSKEY_SET_DAO";

DnskeySetDao::DnskeySetDao()
{
  setNsID(1);
}

DnskeySetDao::~DnskeySetDao()
{

}

bool DnskeySetDao::init(std::string &p_sName,
                        std::string &p_sSrc)
{
  string sName = p_sName;
  transform(sName.begin(), sName.end(), sName.begin(), ::tolower);
  setName(sName);
  setSrc(p_sSrc);

  return true;
}

bool DnskeySetDao::addKey(DnsDnskey &p_oKey)
{
  bool bRet = false;

  string &sName = getName();
  DnsName *pName = p_oKey.get_name();
  if (NULL == pName)
  {
    ps_elog(PSL_DEBUG, "DnsDnskey has NULL name.\n");
  }
  else if (sName != pName->toString())
  {
    ps_elog(PSL_ERROR, "Unable to add key with name '%s' when name is already set to '%s'\n",
           pName->toString().c_str(),
           sName.c_str());
  }
  else
  {
    ostringstream oSS;
    oSS << sName
        << " "
        << p_oKey.ttl()
        << " "
        << p_oKey.get_class()
        << " " 
        << "DNSKEY"
        << " " 
        << p_oKey.getFlags()
        << " " 
        << (unsigned) p_oKey.getProto()
        << " " 
        << (unsigned) p_oKey.getAlgo()
        << " (" 
        << p_oKey.getKey()
        << ") ;";
    m_oKeySet.push_back(oSS.str());
    m_oKeySet.sort();

    bRet = setDnsData();
  }

  return bRet;
}

bool DnskeySetDao::addRrsig(DnsRrsig &p_oSig)
{
  bool bRet = false;

  string &sName = getName();
  DnsName *pName = p_oSig.get_name();
  if (NULL == pName)
  {
    ps_elog(PSL_DEBUG, "DnsRrsig has NULL name.\n");
  }
  else if (sName != pName->toString())
  {
    ps_elog(PSL_ERROR, "Unable to add sig with name '%s' when name is already set to '%s'\n",
           pName->toString().c_str(),
           sName.c_str());
  }
  else
  {
    ostringstream oSS;
    oSS << sName
        << " "
        << p_oSig.ttl()
        << " "
        << p_oSig.get_class()
        << " " 
        << "RRSIG"
        << " " 
        << p_oSig.getTypeCovered()
        << " " 
        << (unsigned) p_oSig.getAlgo()
        << " " 
        << (unsigned) p_oSig.getLabels()
        << " " 
        << p_oSig.getOrigTTL()
        << " " 
        << p_oSig.getExpiration()
        << " " 
        << p_oSig.getInception()
        << " " 
        << p_oSig.getKeyTag()
        << " " 
        << p_oSig.getSignersName()
        << " (" 
        << p_oSig.getSig()
        << ") ;";
    m_oSigSet.push_back(oSS.str());
    m_oSigSet.sort();

    bRet = setDnsData();
  }

  return bRet;
}

bool DnskeySetDao::setDnsData()
{
  bool bRet = false;

  try
  {
    string sData;
    list<string>::iterator oIter;
    for (oIter = m_oKeySet.begin();
         m_oKeySet.end() != oIter;
         oIter++)
    {
      sData += *oIter + "\n";
    }

    for (oIter = m_oSigSet.begin();
         m_oSigSet.end() != oIter;
         oIter++)
    {
      sData += *oIter + "\n";
    }

    setData(sData.c_str(), sData.size());

    bRet = true;
  }
  catch (...)
  {
    ps_elog(PSL_CRITICAL, "Caught exception!\n");
    bRet = false;
  }

  return bRet;
}

size_t DnskeySetDao::getNumKeys()
{
  return m_oKeySet.size();
}

bool DnskeySetDao::serialize()
{
  bool bRet = false;

  time_t tNow = time(NULL);
  deserialize();
  setDate(tNow);
  bRet = RawDataDao::serialize();

  return bRet;
}

DnskeySetDao *DnskeySetDao::dup()
{
  return new DnskeySetDao();
}
