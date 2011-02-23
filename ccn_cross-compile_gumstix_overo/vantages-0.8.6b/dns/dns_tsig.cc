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

#include <stdio.h>
#include <netinet/in.h>
#include <openssl/hmac.h>
#include <string.h>

#include <fstream>

#include "dns_tsig.h"
#include "dns_name.h"
#include "dns_packet.h"
#include "base64.h"
#include "dns_err.h"
#include "dns_defs.h"

using namespace std;

const char *DnsTsig::s_kszMd5HmacName = "hmac-md5.sig-alg.reg.int.";

DnsTsig::DnsTsig()
  : DnsRR(DNS_RR_TSIG),
    m_tTime(0),
    m_uFudge(DNS_TSIG_DEFAULT_FUDGE),
    m_uMacSize(0),
    m_pMac(NULL),
    m_uOrigID(0),
    m_uError(0),
    m_uOtherLen(0),
    m_pOtherData(NULL),
    m_pKey(NULL),
    m_uKeyLen(0)
{
  set_class(DNS_CLASS_ANY);
}

DnsTsig::DnsTsig(DnsTsig &p_oRHS)
  : DnsRR(p_oRHS),
    m_tTime(0),
    m_uFudge(DNS_TSIG_DEFAULT_FUDGE),
    m_uMacSize(0),
    m_pMac(NULL),
    m_uOrigID(0),
    m_uError(0),
    m_uOtherLen(0),
    m_pOtherData(NULL),
    m_pKey(NULL),
    m_uKeyLen(0)
{
  set_class(DNS_CLASS_ANY);
  *this = p_oRHS;
}

DnsTsig::~DnsTsig()
{
  setMac(NULL, 0);
  setOtherData(NULL, 0);
  setKey(NULL, 0);
}

const DnsTsig &DnsTsig::operator=(DnsTsig &p_oRHS)
{
  DnsRR::operator=(p_oRHS);
  setAlgoName(p_oRHS.getAlgoName());
  setTime(p_oRHS.getTime());
  setFudge(p_oRHS.getFudge());
  setMac(p_oRHS.getMac(), p_oRHS.getMacSize());
  setOrigID(p_oRHS.getOrigID());
  setError(p_oRHS.getError());
  setOtherData(p_oRHS.getOtherData(), p_oRHS.getOtherLen());
  setKey(p_oRHS.getKey(), p_oRHS.getKeyLen());

  return *this;
}

bool DnsTsig::operator==(DnsTsig &p_oRHS)
{
  return false;
}

bool DnsTsig::rdata_valid()
{
  return (getAlgoName() == s_kszMd5HmacName);
}

std::string &DnsTsig::getAlgoName()
{
  return m_sAlgoName;
}

void DnsTsig::setAlgoName(char *p_szAlgoName)
{
  setAlgoName((const char *) p_szAlgoName);
}

void DnsTsig::setAlgoName(const char *p_kszAlgoName)
{
  if (NULL == p_kszAlgoName)
  {
    dns_log("Unable to set algo name with NULL param.\n");
    DnsError::getInstance().setError("Unable to set algo name with NULL param.");
  }
  else
  {
    string sAlgo = p_kszAlgoName;
    setAlgoName(sAlgo);
  }
}

void DnsTsig::setAlgoName(std::string &p_sAlgoName)
{
  DnsName oName(p_sAlgoName);
  m_sAlgoName = oName.toString();
}

time_t DnsTsig::getTime()
{
  return m_tTime;
}

void DnsTsig::setTime(time_t p_tTime)
{
  m_tTime = p_tTime;
}

uint16_t DnsTsig::getFudge()
{
  return m_uFudge;
}

void DnsTsig::setFudge(uint16_t p_uFudge)
{
  m_uFudge = p_uFudge;
}

uint16_t DnsTsig::getMacSize()
{
  return m_uMacSize;
}

u_char *DnsTsig::getMac()
{
  return m_pMac;
}

void DnsTsig::setMac(u_char *p_pMac, uint16_t p_uMacSize)
{
  if (NULL != m_pMac);
  {
    m_uMacSize = 0;
    delete[] m_pMac;
    m_pMac = NULL;
  }

  if (NULL != p_pMac && 0 != p_uMacSize)
  {
    m_uMacSize = p_uMacSize;
    m_pMac = new u_char[m_uMacSize];
    memcpy(m_pMac, p_pMac, m_uMacSize);
  }
}

uint16_t DnsTsig::getOrigID()
{
  return m_uOrigID;
}

void DnsTsig::setOrigID(uint16_t p_uOrigID)
{
  m_uOrigID = p_uOrigID;
}

uint16_t DnsTsig::getError()
{
  return m_uError;
}

void DnsTsig::setError(uint16_t p_uError)
{
  m_uError = p_uError;
}

uint16_t DnsTsig::getOtherLen()
{
  return m_uOtherLen;
}

u_char *DnsTsig::getOtherData()
{
  return m_pOtherData;
}

void DnsTsig::setOtherData(u_char *p_pOtherData, uint16_t p_uOtherLen)
{
  if (NULL != m_pOtherData)
  {
    m_uOtherLen = 0;
    delete[] m_pOtherData;
    m_pOtherData = NULL;
  }

  if (NULL != p_pOtherData && 0 < p_uOtherLen)
  {
    m_uOtherLen = p_uOtherLen;
    m_pOtherData = new u_char[m_uOtherLen];
    memcpy(m_pOtherData, p_pOtherData, m_uOtherLen);
  }
}

size_t DnsTsig::getKeyLen()
{
  return m_uKeyLen;
}

u_char *DnsTsig::getKey()
{
  return m_pKey;
}

void DnsTsig::setKey(u_char *p_pKey, size_t p_uKeyLen)
{
  if (NULL != m_pKey)
  {
    m_uKeyLen = 0;
    delete m_pKey;
    m_pKey = NULL;
  }

  if (NULL != p_pKey && 0 < p_uKeyLen)
  {
    m_uKeyLen = p_uKeyLen;
    m_pKey = new u_char[m_uKeyLen];
    memcpy(m_pKey, p_pKey, m_uKeyLen);
  }
}

bool DnsTsig::supportedAlgo(std::string &p_sAlgoName)
{
  return (p_sAlgoName == s_kszMd5HmacName);
}

DnsTsig *DnsTsig::dup()
{
  return new DnsTsig();
}

void DnsTsig::printRData()
{
  fprintf(stdout, "%s\t%d\t%d\t%d\t%s\t%d\t%d\t%d\n",
          getAlgoName().c_str(),
          (int) getTime(),
          getFudge(),
          getMacSize(),
          base64_encode(getMac(), getMacSize()).c_str(),
          getOrigID(),
          getError(),
          getOtherLen());
}

bool DnsTsig::parseRData(u_char *p_pMsg,
                         size_t p_uMsgLen,
                         u_char *p_pRData,
                         size_t p_uRDataLen)
{
  bool bRet = false;

  size_t uOffset = (size_t) (p_pRData - p_pMsg);

  if (NULL == p_pMsg)
  {
    dns_log("No message specified.\n");
  }
  else if (0 == p_uMsgLen)
  {
    dns_log("Message length cannot be 0\n");
  }
  else if (NULL == p_pRData)
  {
    dns_log("Rdata cannot be NULL.\n");
  }
  else if (0 == p_uRDataLen)
  {
    dns_log("rdata length cannot be 0\n");
  }
  else if (p_pRData <= p_pMsg)
  {
    dns_log("RData pointer was below message: 0x%lx <= 0x%lx\n", (unsigned long) p_pRData, (unsigned long) p_pMsg);
  }
  else if (uOffset > 4096)
  {
    dns_log("Calculated offset %u is too large.\n", (unsigned) uOffset);
  }
  else if (p_uRDataLen <= 16)
  {
    dns_log("rdata length was too short: %u\n", (unsigned) p_uRDataLen);
  }
  else
  {
    DnsName *pName = DnsName::from_wire(p_pMsg, p_uMsgLen, uOffset);
    if (NULL == pName)
    {
      dns_log("No name was able to be extraced.\n");
    }
    else
    {
      string sName = pName->toString();
      delete pName;

      if (!supportedAlgo(sName))
      {
        DnsError::getInstance().setError("Algorithm is not supported");
        dns_log("Algorithm '%s' is not supported\n", sName.c_str());
      }
      else
      {
dns_log("GOT ALGO: '%s'\n", sName.c_str());
        setAlgoName(sName);

        dns_tsig_header_1_t tHeader1;
        dns_tsig_header_2_t tHeader2;

        memcpy(&tHeader1, &(p_pMsg[uOffset]), sizeof(tHeader1));
        tHeader1.m_uTime = ntohl(tHeader1.m_uTime);
        tHeader1.m_uFudge = ntohs(tHeader1.m_uFudge);
        tHeader1.m_uMacSize = ntohs(tHeader1.m_uMacSize);
        uOffset += sizeof(tHeader1);

        setTime(tHeader1.m_uTime);
        setFudge(tHeader1.m_uFudge);

        setMac(&(p_pMsg[uOffset]), tHeader1.m_uMacSize);
        uOffset += tHeader1.m_uMacSize;

        memcpy(&tHeader2, &(p_pMsg[uOffset]), sizeof(tHeader2));
        tHeader2.m_uOrigID = ntohs(tHeader2.m_uOrigID);
        tHeader2.m_uError = ntohs(tHeader2.m_uError);
        tHeader2.m_uOtherLen = ntohs(tHeader2.m_uOtherLen);
        uOffset += sizeof(tHeader2);

        setOrigID(tHeader2.m_uOrigID);
        setError(tHeader2.m_uError);

        setOtherData(&(p_pMsg[uOffset]), tHeader2.m_uOtherLen);
        uOffset += tHeader2.m_uOtherLen;

        bRet = true;
      }
    }
  }

  return bRet;
}

bool DnsTsig::calc(DnsPacket &p_oPkt, bool p_bSetTime /*= true*/)
{
  bool bRet = false;

  // Room for a max message size + a huge TSIG RR
  u_char pBuff[DNS_TSIG_MAX_BUFSIZ];
  memset(pBuff, 0, sizeof(pBuff));
  int iOffset = 0;
  DnsName *pName = get_name();
  DnsName oAlgoName(getAlgoName());

  if (!supportedAlgo(getAlgoName()))
  {
    dns_log("Unable to sign message with unknown algo: '%s'\n", getAlgoName().c_str());
    DnsError::getInstance().setError("Unable to sign message with unknown algo");
  }
  else if (p_oPkt.hasEmbeddedTsig())
  {
    dns_log("Unable to sign message with embedded TSIG RR.\n");
    DnsError::getInstance().setError("Unable to sign message with embedded TSIG RR.");
  }
  else if (NULL == pName)
  {
    dns_log("TSIG RR does not have name?\n");
  }
  else
  {
    iOffset = 0;


    setOrigID(p_oPkt.getHeader().id());

    if (p_bSetTime)
    {
      setTime(time(NULL));
    }

    if (getMacSize() > 0)
    {
      uint16_t uSize = htons(getMacSize());
//      memcpy(pBuff, &uSize, sizeof(uSize));
//      iOffset += sizeof(uSize);
      *(uint16_t *)&(pBuff[iOffset]) = uSize;//htons(getMacSize());
      iOffset += 2;
      memcpy(&(pBuff[iOffset]), getMac(), getMacSize());
      iOffset += getMacSize();
    }

    int iErr = 0;
    if (0 == (iErr = p_oPkt.toWire(&(pBuff[iOffset]), DNS_TSIG_MAX_BUFSIZ - iOffset, false)))
    {
      dns_log("Unable to serialize packet.\n");
    }
    else
    {
      iOffset += iErr;

      dns_tsig_vars_t tVars;
      tVars.m_uTimePadding = 0;
      tVars.m_uTime = htonl(getTime());
      tVars.m_uFudge = htons(getFudge());
      tVars.m_uError = htons(getError());
      tVars.m_uOtherLen = htons(getOtherLen());

      pName->to_wire_canonical(&(pBuff[iOffset]), pName->length());
      iOffset += pName->length();
      *(uint16_t *)&(pBuff[iOffset]) = htons(get_class());
      iOffset += 2;

      *(uint32_t *)&(pBuff[iOffset]) = htonl(ttl());
      iOffset += 4;

      oAlgoName.to_wire_canonical(&(pBuff[iOffset]), oAlgoName.length());
      iOffset += oAlgoName.length();

      memcpy(&(pBuff[iOffset]), &tVars, sizeof(tVars));
      iOffset += sizeof(tVars);
      if (getOtherLen() > 0)
      {
        memcpy(&(pBuff[iOffset]), getOtherData(), getOtherLen());
        iOffset += getOtherLen();
      }

      HMAC_CTX tHmacCtx;
      u_char pMac[BUFSIZ];
      unsigned int iMacSize = 0;

      memset(pMac, 0, BUFSIZ);
      HMAC_CTX_init(&tHmacCtx);
      HMAC_Init_ex(&tHmacCtx, getKey(), getKeyLen(), EVP_md5(), NULL);
      HMAC_Update(&tHmacCtx, pBuff, iOffset);
      HMAC_Final(&tHmacCtx, pMac, &iMacSize);
      HMAC_CTX_cleanup(&tHmacCtx);

      setMac(pMac, iMacSize);
      setRData();

      bRet = true;
    }
/*
bRet = true;
*/
  }

  return bRet;
}

bool DnsTsig::verify(DnsPacket &p_oPkt, u_char *p_pPreviousMac, size_t p_uPreviousMacSize)
{
  bool bRet = false;

  u_char *pNewMac = getMac();
  uint16_t uNewMacSize = getMacSize();

  if (NULL == pNewMac)
  {
    dns_log("Message returned without a MAC in it.\n");
  }
  else if (0 == uNewMacSize)
  {
    dns_log("Message returned wit a 0-length MAC in it.\n");
  }
  else
  {
    u_char *pBuff = new u_char[uNewMacSize];
    memcpy(pBuff, pNewMac, uNewMacSize);

    setMac(p_pPreviousMac, p_uPreviousMacSize);

    if (!calc(p_oPkt, false))
    {
      dns_log("Unable to calculate MAC.\n");
    }
    else if (getMacSize() != uNewMacSize)
    {
      dns_log("Calculated MAC size differs from sent size: %u != %u\n", getMacSize(), uNewMacSize);
      DnsError::getInstance().setError("Calculated MAC size differs from sent size");
    }
    else if (memcmp(getMac(), pBuff, uNewMacSize) != 0)
    {
      dns_log("Calculated MAC and sent MAC differ in content:\n\t%s\n\t%s\n",
              base64_encode(getMac(), getMacSize()).c_str(),
              base64_encode(pBuff, uNewMacSize).c_str());
      DnsError::getInstance().setError("Calculated MAC and sent MAC differ in content.");
      p_oPkt.print();
    }
    else
    {
      dns_log("Verified MAC.\n");
      setMac(pBuff, uNewMacSize);
      bRet = true;
    }

    delete[] pBuff;
  }

  return bRet;
}

bool DnsTsig::loadKeyFromFile(char *p_szFile)
{
  return loadKeyFromFile((const char *) p_szFile);
}

bool DnsTsig::loadKeyFromFile(const char *p_kszFile)
{
  bool bRet = false;

  if (NULL == p_kszFile)
  {
    dns_log("Unable to parse file with NULL name.\n");
    DnsError::getInstance().setError("Unable to parse file with NULL name.");
  }
  else
  {
    string sFile = p_kszFile;
    size_t u = 0;
    if (string::npos != (u = sFile.rfind("/")))
    {
      sFile = sFile.substr(u + 1);
    }

    if (0 == sFile.find("K")
        && string::npos != (u = sFile.find("+")))
    {
      string sName = sFile.substr(1, u - 1);
      DnsName oName(sName);
      set_name(oName);
dns_log("Setting name: '%s'\n", sName.c_str());
    }

    vector<string> oLines;
    try
    {
      string sLine;
      ifstream oKeyFile(p_kszFile, ios_base::in);
      while (getline(oKeyFile, sLine, '\n'))
      {
        oLines.push_back(sLine);
      }

      size_t uCount = oLines.size();
      if (1 == uCount)
      {
        bRet = parsePublicKey(oLines);
      }
      else if (3 == uCount)
      {
        bRet = parsePrivateKey(oLines);
      }
      else
      {
        dns_log("Unable to identify file with %u lines.\n", (unsigned) uCount);
        DnsError::getInstance().setError("Unable to identify file with wrong number of lines.");
      }
    }
    catch(...)
    {
      dns_log("Caught unknown exceotion.\n");
      DnsError::getInstance().setError("Caught unknown exceotion.");
      bRet = false;
    }
  }

  return bRet;
}

bool DnsTsig::loadKeyFromFile(std::string &p_sFile)
{
  return loadKeyFromFile((const char *) p_sFile.c_str());
}

bool DnsTsig::setRData()
{
  bool bRet = false;

  u_char pBuff[BUFSIZ];
  memset(pBuff, 0, BUFSIZ);

  size_t uOffset = 0;
  dns_tsig_header_1_t tHeader1;
  dns_tsig_header_2_t tHeader2;
  memset(&tHeader1, 0, sizeof(tHeader1));
  memset(&tHeader2, 0, sizeof(tHeader2));

  string &sName = getAlgoName();
  DnsName oAlgo(sName);
  oAlgo.to_wire_canonical(pBuff, oAlgo.length());
  uOffset += oAlgo.length();

  tHeader1.m_uTime = htonl(getTime());
  tHeader1.m_uFudge = htons(getFudge());
  tHeader1.m_uMacSize = htons(getMacSize());
  memcpy(&(pBuff[uOffset]), &tHeader1, sizeof(tHeader1));
  uOffset += sizeof(tHeader1);

  u_char *pMac = getMac();
  uint16_t uMacSize = getMacSize();
  if (NULL != pMac && 0 < uMacSize)
  {
    memcpy(&(pBuff[uOffset]), pMac, uMacSize);
    uOffset += uMacSize;
  }

  tHeader2.m_uOrigID = htons(getOrigID());
  tHeader2.m_uError = htons(getError());
  tHeader2.m_uOtherLen = htons(getOtherLen());
  memcpy(&(pBuff[uOffset]), &tHeader2, sizeof(tHeader2));
  uOffset += sizeof(tHeader2);

  u_char *pOtherData = getOtherData();
  uint16_t uOtherLen = getOtherLen();
  if (NULL != pOtherData && 0 < uOtherLen)
  {
    memcpy(&(pBuff[uOffset]), pOtherData, uOtherLen);
    uOffset += uOtherLen;
  }

  set_rdata(pBuff, uOffset);
  bRet = true;

  return bRet;
}


bool DnsTsig::parsePrivateKey(std::vector<std::string> &p_oLines)
{
  bool bRet = false;

  string &sFormat = p_oLines[0];
  string &sAlgo = p_oLines[1];
  string &sKeyLine = p_oLines[2];

  if (0 != sFormat.find("Private-key-format"))
  {
    dns_log("Unable to find key format line in file: '%s'\n", sFormat.c_str());
    DnsError::getInstance().setError("Unable to find key format line in file");
  }
  else if (0 != sAlgo.find("Algorithm"))
  {
    dns_log("Unable to find algorithm line in file: '%s'\n", sAlgo.c_str());
    DnsError::getInstance().setError("Unable to find algorithm line in file.");
  }
  else if (10 != sAlgo.find(" 157 "))
  {
    dns_log("Algorithm is not 157 (MD5-HMAC) in file '%s'\n", sAlgo.c_str());
    DnsError::getInstance().setError("Algorithm is not 157 (MD5-HMAC) in file");
  }
  else if (0 != sKeyLine.find("Key: "))
  {
    dns_log("Unable to find key line in file\n");
    DnsError::getInstance().setError("Unable to find key line in file");
  }
  else
  {
    string sKey = sKeyLine.substr(5);
    dns_log("Found key '%s'\n", sKey.c_str());
    std::vector<unsigned char> oKeyBytes = base64_decode(sKey);

    u_char pBuff[BUFSIZ];
    memset(pBuff, 0, BUFSIZ);
    size_t uKeySize = oKeyBytes.size();
    if (uKeySize > BUFSIZ)
    {
      dns_log("Got an oversized key (%u) back.\n", (unsigned) uKeySize);
      DnsError::getInstance().setError("Got an oversized key back.");
    }
    else
    {
      for (size_t u = 0; u < uKeySize; u++)
      {
        pBuff[u] = oKeyBytes[u];
      }

      setKey(pBuff, uKeySize);
      setAlgoName(s_kszMd5HmacName);
      bRet = true;
    }
  }

  return bRet;
}

bool DnsTsig::parsePublicKey(std::vector<std::string> &p_oLines)
{
  bool bRet = false;

  string sLine = p_oLines.front();
  vector<string> oTokens;
  oTokens.resize(7);

  size_t u = 0;
  int i = 0;
  for (i = 0;
       i < 6
       && string::npos != (u = sLine.find(" "));
       i++)
  {
    oTokens.push_back(sLine.substr(0, u));
    sLine = sLine.substr(u + 1);
  }

  if (sLine.empty())
  {
    dns_log("Not enough tokens in line.  Only found: %d\n", i);
  }
  else if (6 != i)
  {
    dns_log("Not enough tokens in line... missing key.\n");
  }
  else if (oTokens[5] != "157")
  {
    dns_log("Algorithm not supported.  Looking for 157 instead of '%s'\n", oTokens[5].c_str());
  }
  else
  {
    string &sKey = oTokens.back();
    dns_log("Found key '%s'\n", sKey.c_str());
    std::vector<unsigned char> oKeyBytes = base64_decode(sKey);

    u_char pBuff[BUFSIZ];
    memset(pBuff, 0, BUFSIZ);
    size_t uKeySize = oKeyBytes.size();
    if (uKeySize > BUFSIZ)
    {
      dns_log("Got an oversized key (%u) back.\n", (unsigned) uKeySize);
      DnsError::getInstance().setError("Got an oversized key back.");
    }
    else
    {
      for (size_t u = 0; u < uKeySize; u++)
      {
        pBuff[u] = oKeyBytes[u];
      }

      setKey(pBuff, uKeySize);
      setAlgoName(s_kszMd5HmacName);
      DnsName oName(oTokens[0]);
      set_name(oName);
dns_log("Setting name: '%s'\n", oTokens[0].c_str());

      bRet = true;
    }
  }

  return bRet;
}
