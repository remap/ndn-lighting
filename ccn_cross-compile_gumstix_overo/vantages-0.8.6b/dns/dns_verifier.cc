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
#include <netinet/in.h>

#include <openssl/ssl.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/md5.h>

#include "dns_verifier.h"
#include "dns_dnskey.h"
#include "dns_rrsig.h"
#include "dns_rr.h"
#include "dns_name.h"
#include "dns_defs.h"

using namespace std;

DnsVerifier::algo_map_t DnsVerifier::s_oAlgoMap;
bool DnsVerifier::s_bInit = false;

DnsVerifier::DnsVerifier()
{
  ERR_load_crypto_strings();

  if (!s_bInit)
  {
    s_bInit = true;
    s_oAlgoMap.clear();
    s_oAlgoMap[5] = (const char *) "SHA1withRSA";
    s_oAlgoMap[7] = (const char *) "SHA1withRSA";
#ifndef _DNS_NO_SHA256
    s_oAlgoMap[8] = (const char *) "SHA256withRSA";
#endif
  }
}

DnsVerifier::~DnsVerifier()
{

}

const char *DnsVerifier::getAlgoName(int p_iAlgo)
{
  const char *pRet = NULL;

  algo_iter_t tIter = s_oAlgoMap.find(p_iAlgo);
  if (s_oAlgoMap.end() != tIter)
  {
    pRet = tIter->second;
  }

  return pRet;
}

bool DnsVerifier::verify(RRList_t &p_oKeyList, RRList_t &p_oRRset)
{
  bool bRet = false;

  RRList_t oKeyList;
  string sName;

  for (RRIter_t tIter = p_oKeyList.begin();
       p_oKeyList.end() != tIter;
       tIter++)
  {
    DnsRR *pRR = *tIter;
    if (sName == "")
    {
      sName = pRR->get_name()->verifName();
dns_log("GOT VERIF NAME '%s'\n", sName.c_str());
    }

    if (DNS_RR_DNSKEY == pRR->type())
    {
      DnsDnskey *pKey = static_cast<DnsDnskey *>(pRR);
      int iAlgo = pKey->getAlgo();
      const char *szAlgo = getAlgoName(iAlgo);
      if (NULL == szAlgo)
      {
        dns_log("Unknow algo type for DNSKEY: %d\n", pKey->getAlgo());
      }
      else if (sName != pRR->get_name()->verifName())
      {
        dns_log("Unable to add key with name: '%s' when key set is being built with name: '%s'\n",
                pRR->get_name()->verifName().c_str(),
                sName.c_str());
      }
      else
      {
        oKeyList.push_back(pKey);
      }
    }
  }

  if (oKeyList.empty())
  {
    dns_log("Unable to verify w/o any keys.\n");
  }
  else
  {
    int iType = 0;
    int iClass = 0;
    bool bSent = true;

    RRList_t oDataRRs;
    RRList_t oSigRRs;

    for (RRIter_t tIter = p_oRRset.begin();
         bSent
         && p_oRRset.end() != tIter;
         tIter++)
    {
      DnsRR *pRR = *tIter;
      if (DNS_RR_RRSIG == pRR->type())
      {
        DnsRrsig *pRRSIG = static_cast<DnsRrsig *>(pRR);
        uint32_t uExp = pRRSIG->getExpiration();
        uint32_t uIncep = pRRSIG->getInception();
        time_t tNow = time(NULL);

        if (uExp < uIncep)
        {
          dns_log("Inception cannot come after expiration: %u > %u\n", (unsigned) uIncep, (unsigned) uExp);
        }
        else if (uIncep > (uint32_t) tNow)
        {
          dns_log("Inception cannot be in the future: %u > %u\n", (unsigned) uIncep, (unsigned) tNow);
        }
        else if (uExp < (uint32_t) tNow)
        {
          dns_log("Signature has expired: %u < %u\n", (unsigned) uExp, (unsigned) tNow);
        }
        else
        {
          oSigRRs.push_back(pRR);
        }
      }
      else
      {
        if (0 == iType)
        {
          iType = pRR->type();
          iClass = pRR->get_class();
        }

        string sCandName = pRR->get_name()->verifName();
        if (pRR->type() != iType)
        {
          dns_log("Type mismatch %d != %d\n", pRR->type(), iType);
          bSent = false;
        }
        else if (pRR->get_class() != iClass)
        {
          dns_log("Class mismatch %d != %d\n", pRR->get_class(), iClass);
          bSent = false;
        }
        else if (!endsWith(sCandName, sName))
        {
          dns_log("Unable to add RR with different name from key set: '%s' != '%s'\n", pRR->get_name()->verifName().c_str(), sName.c_str());
          bSent = false;
        }
        else
        {
          oDataRRs.push_back(pRR);
        }
      }
    }

    if (bSent)
    {
      u_char pBuff[4096];
      for (RRIter_t tIter = oSigRRs.begin();
           !bRet
           && oSigRRs.end() != tIter;
           tIter++)
      {
        memset(pBuff, 0, 4096);
        size_t uSize = 0;
        if (0 == (uSize = flatten(oDataRRs, *(static_cast<DnsRrsig *>(*tIter)), pBuff, 4096)))
        {
          dns_log("Unable to flatten buffer.\n");
          break;
        }
        else
        {
          dns_log("flattened to size %u.\n", (unsigned) uSize);
          for (RRIter_t tKeyIter = oKeyList.begin();
               oKeyList.end() != tKeyIter;
               tKeyIter++)
          {
#ifdef _DNS_DEBUG
            size_t u = 0;
            dns_log("Character dump of verification data:\n");
            for (u = 0; u < uSize; u++)
            {
              if (isalpha(pBuff[u]))
              {
                fprintf(stderr, "%c", pBuff[u]);
              }
              else
              {
                fprintf(stderr, "%02x", pBuff[u]);
              }
            }
            fprintf(stderr, "\n=======\n");
            dns_log("Hex dump of verification data:\n");
            for (u = 0; u < uSize; u++)
            {
              fprintf(stderr, "%02x", pBuff[u]);
            }
            fprintf(stderr, "\n");
#endif

            if (verify(*static_cast<DnsDnskey *>(*tKeyIter), *(static_cast<DnsRrsig *>(*tIter)), pBuff, uSize))
            {
              bRet = true;
              break;
            }
          }
        }
      }
    }
  }

  return bRet;
}

bool DnsVerifier::endsWith(std::string &p_sCandidateName, std::string &p_sBaseName)
{
  size_t uPos = p_sCandidateName.rfind(p_sBaseName);
  return (string::npos != uPos && (uPos + p_sBaseName.size()) == p_sCandidateName.size());
}

bool DnsVerifier::verify(DnsDnskey &p_oKey, DnsRrsig &p_oRrsig, u_char *p_pBuff, size_t p_uLen)
{
  bool bRet = false;

  int iErr = 0;
  EVP_PKEY *pKey = EVP_PKEY_new();

  iErr = EVP_PKEY_assign_RSA(pKey, extractRSA(p_oKey.getBinKey(), p_oKey.getBinKeyLen()));//, p_oKey.getBinKeyLen());
  EVP_MD_CTX pEvpCtx;
  EVP_MD_CTX_init(&pEvpCtx);

  int iAlgo = p_oKey.getAlgo();
  const char *szAlgo = getAlgoName(iAlgo);
  if (NULL == szAlgo)
  {
    dns_log("Algo %d not supported.\n", p_oKey.getAlgo());
  }
  else if (NULL == p_pBuff)
  {
    dns_log("Can't verify NULL buffer.\n");
  }
  else if (0 == p_uLen)
  {
    dns_log("Unable to verify 0 len buffer.\n");
  }
  else if (1 != iErr)
  {
    char pBuffer[120];
    ERR_error_string(ERR_get_error(), pBuffer);
    dns_log("RSA failure: '%s'\n", pBuffer);
  }
  else
  {
    if (5 == iAlgo || 7 == iAlgo)
    {
      if (1 != (iErr = EVP_VerifyInit(&pEvpCtx, EVP_sha1())))
      {
        dns_log("Unable to verify init.\n");
      }
      else if (1 != (iErr = EVP_VerifyUpdate(&pEvpCtx, p_pBuff, p_uLen)))
      {
        dns_log("Unable to verify update.\n");
      }
      else if (1 != (iErr = EVP_VerifyFinal(&pEvpCtx, p_oRrsig.getBinSig(), p_oRrsig.getBinSigLen(), pKey)))
      {
        char pBuffer[120];
        ERR_error_string(ERR_get_error(), pBuffer);
        dns_log("Unable to verify final: '%s'\n", pBuffer);
      }
      else
      {
        bRet = true;
      }
    }
#ifndef _DNS_NO_SHA256
    else if (8 == iAlgo)
    {
      if (1 != (iErr = EVP_VerifyInit(&pEvpCtx, EVP_sha256())))
      {
        dns_log("Unable to verify init.\n");
      }
      else if (1 != (iErr = EVP_VerifyUpdate(&pEvpCtx, p_pBuff, p_uLen)))
      {
        dns_log("Unable to verify update.\n");
      }
      else if (1 != (iErr = EVP_VerifyFinal(&pEvpCtx, p_oRrsig.getBinSig(), p_oRrsig.getBinSigLen(), pKey)))
      {
        char pBuffer[120];
        ERR_error_string(ERR_get_error(), pBuffer);
        dns_log("Unable to verify final: '%s'\n", pBuffer);
      }
      else
      {
        bRet = true;
      }
    }
#endif
  }

  EVP_MD_CTX_cleanup(&pEvpCtx);
  EVP_PKEY_free(pKey);

  return bRet;
}

size_t DnsVerifier::flatten(RRList_t &p_oDataRRs, DnsRrsig &p_oRrsig, u_char *p_pBuff, size_t p_uBuffLen)
{
  size_t uRet = 0;

  if (NULL == p_pBuff)
  {
    dns_log("Unable to use NULL buffer.\n");
  }
  else if (0 == p_uBuffLen)
  {
    dns_log("Unable to use 0 buff len.\n");
  }
  else
  {
    DnsBits_t oSigBits;
    size_t uOffset = p_oRrsig.verificationRData(oSigBits);
    std::copy(oSigBits.begin(), oSigBits.end(), p_pBuff);

    dns_log("Got a signature of length: %u\n", (unsigned) uOffset);

    list< vector<u_char> > oDataList;
    RRIter_t tIter;
    for (tIter = p_oDataRRs.begin();
         p_oDataRRs.end() != tIter;
         tIter++)
    {
      DnsBits_t oVec;
      size_t uTmp = (*tIter)->verificationWireFormat(oVec);
      if (0 == uTmp)
      {
        dns_log("Unable to prepare data.\n");
        break;
      }
      else
      {
        dns_log("Added %u bytes of RR data.\n", (unsigned) uTmp);
        oDataList.push_back(oVec);
      }
    }

    if (p_oDataRRs.end() == tIter)
    {
      oDataList.sort();

      for (list< vector<u_char> >::iterator tVecIter = oDataList.begin();
           oDataList.end() != tVecIter;
           tVecIter++)
      {

        DnsBits_t &oVecRef = *tVecIter;
        size_t uTmp = oVecRef.size();
        if (uTmp + uOffset > p_uBuffLen)
        {
          dns_log("Not enough room in buffer: %u > %u\n", (unsigned) (uTmp + uOffset), (unsigned) p_uBuffLen);
          break;
        }
        else
        {
          std::copy(oVecRef.begin(), oVecRef.end(), &(p_pBuff[uOffset]));
          uOffset += uTmp;
        }
        dns_log("Offset is: %u\n", (unsigned) uOffset);
      }

      uRet = uOffset;
    }
  }

  dns_log("Total length is: %u\n", (unsigned) uRet);
  return uRet;
}

RSA *DnsVerifier::extractRSA(u_char *p_pBuff, size_t p_uLen)
{
  RSA *pRet = NULL;

  if (NULL == p_pBuff)
  {
    dns_log("Cannot find key in NULL buffer.\n");
  }
  else if (0 == p_uLen)
  {
    dns_log("Unable to find key with 0 len buffer.\n");
  }
  else if (0 == p_pBuff[0] && p_uLen < 3)
  {
    dns_log("Buffer seems too small to account for exponent?\n");
  }
  else
  {
    uint16_t uOffset = 0;
    uint16_t uExp = 0;

    if (0 == p_pBuff[0])
    {
      uint16_t uTmp = 0;
      memmove(&uTmp, p_pBuff + 1, 2);
      uExp = ntohs(uTmp);
      uOffset = 3;
    }
    else
    {
      uExp = p_pBuff[0];
      uOffset = 1;
    }

    // The offset and the exponent should not 
    // exceed the actual buffer's length
    if (p_uLen < (size_t) (uOffset + uExp + 1))
    {
      dns_log("Buffer is not large enough to contain the implied key: %u < %u\n",
              (unsigned) p_uLen,
              (unsigned) (uOffset + uExp + 1));
    }
    else
    {
      BIGNUM *pExp = BN_new();
      (void) BN_bin2bn(&(p_pBuff[uOffset]), (int) uExp, pExp);
      uOffset += uExp;

      BIGNUM *pMod = BN_new();
      (void) BN_bin2bn(&(p_pBuff[uOffset]), (int) (p_uLen - uOffset), pMod);

      pRet = RSA_new();
      pRet->n = pMod;
      pRet->e = pExp;
    }
  }

  return pRet;
}

bool DnsVerifier::verifyDs(DnsDs &p_oDs, RRList_t &p_oKeyList) //ds signs a key in the set
{
  RRIter_t oIter;
  for (oIter = p_oKeyList.begin();
       oIter != p_oKeyList.end();
       oIter++)
  {
    if (verifyDs(p_oDs, (DnsDnskey*)*oIter))
      return true;
  }
  return false;
}

bool DnsVerifier::verifyDs(DnsDs &p_oDs, DnsDnskey *p_oKey)
{
  /******
  digest = digest_algorithm( DNSKEY owner name | DNSKEY RDATA )
  DNSKEY RDATA = ( Flags | Protocol | Algorithm | Public Key )
  
  The size of the digest may vary depending on the digest algorithm and
  DNSKEY RR size.  As of the time of this writing, the only defined
  digest algorithm is SHA-1, which produces a 20 octet digest.
  *****/

  
  //build the buffer

  if (&p_oDs == NULL)
    return false;
  if (p_oKey == NULL)
    return false;


  //We only support basich sha1 hashing algorithm...
  if (1 != p_oDs.getAlgo())
  {
    dns_log("Unsupported DS algorithm: %u\n", p_oDs.getAlgo());
    return false;
  }

  DnsBits_t oBits;
  oBits.clear();

  DnsName oName = *p_oKey->get_name();
  u_char* pRData = p_oKey->get_rdata();

  u_char pBuff[255];
  memset(pBuff, 0, 255);
  int iLen = oName.to_wire_canonical(pBuff, 255);
  
  for (int i = 0; i < iLen; i++)
  {
    oBits.push_back(pBuff[i]);
  }
 
  for (size_t u = 0; u < p_oKey->get_rdlen(); u++)
  {
    oBits.push_back(pRData[u]);
  }

  unsigned int iDBLen = iLen + p_oKey->get_rdlen();
  u_char dataBuff [iDBLen];
  std::copy(oBits.begin(), oBits.end(), dataBuff);


  /*
  //---- 
  // Debugging info
  printf("\nContents of the buffer to be hashed:\n");
  for (int i = 0; i < iLen; i++)
  {
    if (dataBuff[i] < 65)
      fprintf(stdout, "%d", dataBuff[i]);
    else
      fprintf(stdout,"%c", dataBuff[i]);
  }
    
    u_char* pRD = dataBuff + iLen;
    size_t pRDLen = p_oKey->get_rdlen();
    printf(" | %d", (ntohs(*(uint16_t *)pRD)));
    pRD += sizeof(uint16_t);
    pRDLen -= sizeof(uint16_t);

    uint8_t uProto = (ntohs(*(uint16_t *) pRD) >> 8) & 0x00ff;
    printf(" | %d", (uProto));

    uint8_t uAlgo = ntohs(*(uint16_t *) pRD) & 0x00ff;
    printf(" | %d", (uAlgo));

    pRD += sizeof(uint16_t);
    pRDLen -= sizeof(uint16_t);

    std::string sKey = base64_encode((const unsigned char *) pRD, pRDLen);

    printf(" | %s\n", sKey.c_str());//(p_pRData, p_uRDataLen));
  //---- 
  */
  
  //does hash match the ds?
  int iErr = 0;
  SHA_CTX pShaCtx = { 0 };
  unsigned char pDigest[SHA_DIGEST_LENGTH];

  if (1 != (iErr = SHA1_Init(&pShaCtx)))
  {
    dns_log("Unable to init sha1 context: '%s'\n", ERR_error_string(iErr, NULL));
  }
  else if (1 != (iErr = SHA1_Update(&pShaCtx, dataBuff, iDBLen)))
  {
    dns_log("Unable to create SHA1 hash: '%s'\n", ERR_error_string(iErr, NULL));
  }
  else if (1 != (iErr = SHA1_Final(pDigest, &pShaCtx)))
  {
    dns_log("Unable to finalize SHA1 context: '%s'\n", ERR_error_string(iErr, NULL));
  }

  if (0 == memcmp(pDigest, p_oDs.getBinDig(), SHA_DIGEST_LENGTH))
  {
    return true;
  }

  return false;
}
