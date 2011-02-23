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
#include <gpgme.h>
#include <gpg-error.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <locale.h>

#include "gpgme_crypt_mgr.h"
#include "ps_logger.h"
#include "ps_util.h"
#include "dao_factory.h"
#include "friend_dao.h"

using namespace std;

const char *GpgmeCryptMgr::GPGMR_CRYPT_MGR = "GPGMR_CRYPT_MGR";

bool GpgmeCryptMgr::s_bInit = false;

gpgme_error_t bb_cb(void *p_pHook, const char *p_pUidHint, const char *p_pPassphraseInfo, int p_iPrevWasBad, int p_iFd)
{
  int iRet = GPG_ERR_CANCELED;
  const char *szP = (const char *) p_pHook;
//  fprintf(stderr, "AM HERE!!!!!\n");
  if (NULL != szP)// && _strnlen(szP, 1024) > 0)
  {
    iRet = write(p_iFd, p_pHook, _strnlen(szP, 1024));
    iRet = 0;
  }
//  fprintf(stderr, "NOW HERE\n");

  return iRet;
}

GpgmeCryptMgr::GpgmeCryptMgr()
  : m_pGpgmeCtx(NULL)
{

}

GpgmeCryptMgr::~GpgmeCryptMgr()
{
  if (NULL != m_pGpgmeCtx)
  {
    gpgme_signers_clear(m_pGpgmeCtx);
    gpgme_release(m_pGpgmeCtx);
    m_pGpgmeCtx = NULL;
  }
}

bool GpgmeCryptMgr::primeEngine()
{
  bool bRet = false;

  gpg_error_t tErr;
  const char *szVersion = gpgme_check_version(NULL);
  setlocale(LC_ALL, "");
  gpg_err_init();
  gpgme_set_locale(NULL, LC_CTYPE, setlocale(LC_CTYPE, NULL));

  if (GPG_ERR_NO_ERROR != (tErr = gpgme_engine_check_version(GPGME_PROTOCOL_OpenPGP)))
  {
    ps_elog(PSL_CRITICAL, "GPG engine doesn't seem to support OpenPGP protocol: '%s'\n", gpg_strerror(tErr));
  }
  else
  {
    ps_elog(PSL_INFO, "Primed GPGME version \"%s\"\n",
           szVersion);
    s_bInit = true;
    bRet = true;
  }

  return bRet;
}

bool GpgmeCryptMgr::loadFriendKeys()
{
  bool bRet = false;

  DaoList_t oDaoList;
  FriendDao *pDao = static_cast<FriendDao *>(DaoFactory::getInstance().create(FriendDao::s_kszDaoName));

  if (NULL == pDao)
  {
    ps_elog(PSL_CRITICAL, "Unable to create friend DAO.\n");
  }
  else if (!pDao->deserialize(oDaoList))
  {
    ps_elog(PSL_CRITICAL, "Unable to load friend DAO to init GPG.\n");
  }
  else
  {
    for (DaoIter_t tIter = oDaoList.begin();
         oDaoList.end() != tIter;
         tIter++)
    {
      string sKey = static_cast<FriendDao *>((*tIter))->getKey();
      if (!addPubKey(sKey))
      {
        ps_elog(PSL_WARNING, "Unable to add key: '%s'\n", sKey.c_str());
      }
      else
      {
        ps_elog(PSL_DEBUG, "Added key: '%s'\n", sKey.c_str());
      }
/*
*/
    }
    bRet = true;
  }

  PsDao::clearList(oDaoList);
  if (NULL != pDao)
  {
    delete pDao;
    pDao = NULL;
  }

  return bRet;
}

std::string &GpgmeCryptMgr::getHomeDir()
{
//  ps_elog (PSL_CRITICAL, "In getHomeDir, m_sHomeDir = %s\n", m_sHomeDir.c_str());
  return m_sHomeDir;
}

void GpgmeCryptMgr::setHomeDir(const char *p_szHomeDir)
{
  m_sHomeDir = string (p_szHomeDir);
//  ps_elog (PSL_CRITICAL, "m_sHomeDir set to %s\n", m_sHomeDir.c_str());
/*  if (NULL == p_szHomeDir)
  {
    m_sHomeDir = "";
  }
  else
  {
    m_sHomeDir = string (p_szHomeDir);
  }*/
}

bool GpgmeCryptMgr::init()
{
  bool bRet = false;

  bool bSent = false;
  gpgme_engine_info_t pEngInfo = NULL;

  const char *szProto = NULL;
  gpg_error_t tErr;

  if (NULL != m_pGpgmeCtx)
  {
    gpgme_release(m_pGpgmeCtx);
    m_pGpgmeCtx = NULL;
  }

  if (!s_bInit)
  {
    bSent = true;

    if (!primeEngine())
    {
      ps_elog(PSL_CRITICAL, "Unable to prime engine.\n");
    }
  }

  //ps_elog (PSL_CRITICAL, "Just before GPG init stuff, m_sHomeDir = %s\n", m_sHomeDir.c_str());
  //string sHome = getHomeDir ();
  //ps_elog (PSL_CRITICAL, "sHome = %s\n", sHome.c_str());
  if (!s_bInit)
  {
    ps_elog(PSL_CRITICAL, "Unable to init GPG lib.\n");
  }
  else if (GPG_ERR_NO_ERROR != (tErr = gpgme_new(&m_pGpgmeCtx)))
  {
    ps_elog(PSL_CRITICAL, "Unable to get GPG context: '%s'\n", gpg_strerror(tErr));
  }
  else if (GPG_ERR_NO_ERROR != (tErr = gpgme_set_protocol(m_pGpgmeCtx, GPGME_PROTOCOL_OpenPGP)))
  {
    ps_elog(PSL_CRITICAL, "Unable to set protocol to OpenPGP: '%s'\n", gpg_strerror(tErr));
  }
  else if (NULL == (szProto = gpgme_get_protocol_name(GPGME_PROTOCOL_OpenPGP)))
  {
    ps_elog(PSL_CRITICAL, "Unable to get protocol name, but protocol exists for OpenPGP?\n");
  }
  else if (GPG_ERR_NO_ERROR != (tErr = gpgme_get_engine_info(&pEngInfo)))
  {
    ps_elog(PSL_CRITICAL, "Unable to get engine info: '%s'\n", gpg_strerror(tErr));
  }
  else if (GPG_ERR_NO_ERROR != (tErr = gpgme_ctx_set_engine_info(m_pGpgmeCtx,
                                                         GPGME_PROTOCOL_OpenPGP,
                                                         pEngInfo->file_name,
                                                         // m_sHomeDir.c_str())))
                                                         (m_sHomeDir == "") ? pEngInfo->home_dir : m_sHomeDir.c_str())))
  {
    ps_elog(PSL_CRITICAL, "Unable to set engine info in context: '%s'\n", gpg_strerror(tErr));
  }
/*
  else if (bSent && !loadFriendKeys())
  {
    ps_elog(PSL_CRITICAL, "Unable to load friend keys.\n");
  }
*/
  else
  {
    ps_elog(PSL_INFO, "Initialized with GPGME protocol \"%s\", file: \"%s\", home: \"%s\"\n",
           szProto,
           pEngInfo->file_name,
           m_sHomeDir.c_str());
//           (m_sHomeDir == "") ? pEngInfo->home_dir : m_sHomeDir.c_str());
    bRet = true;
  }

  return bRet;
}

std::string &GpgmeCryptMgr::getPP()
{
  return m_sPP;
}

bool GpgmeCryptMgr::setPP(std::string &p_sPP)
{
  m_sPP = p_sPP;

  return true;
}

bool GpgmeCryptMgr::verify(std::string &p_sSigData)
{
  bool bRet = false;

  ssize_t lRet = 0;
  size_t uSigSize = p_sSigData.size();
  gpgme_error_t tErr;
  gpgme_data_t pData = NULL;
  gpgme_data_t pSig = NULL;
  gpgme_verify_result_t pResult = NULL;

  if (GPG_ERR_NO_ERROR != (tErr = gpgme_data_new(&pData)))
  {
    ps_elog(PSL_CRITICAL, "Unable to create data buffer: '%s'\n", gpgme_strerror(tErr));
  }
  else if (GPG_ERR_NO_ERROR != (tErr = gpgme_data_new(&pSig)))
  {
    ps_elog(PSL_CRITICAL, "Unable to create signature buffer: '%s'\n", gpgme_strerror(tErr));
  }
  else if (-1 == (lRet = gpgme_data_write(pSig, p_sSigData.c_str(), uSigSize))
           || uSigSize != (size_t) lRet
           || 0 != (lRet = gpgme_data_seek(pSig, 0, SEEK_SET)))
  {
    ps_elog(PSL_CRITICAL, "Unable to write sig data: %d: %s\n", lRet, strerror(errno));
  }
  else if (GPG_ERR_NO_ERROR != (tErr = gpgme_op_verify(m_pGpgmeCtx, pSig, NULL, pData)))
  {
    ps_elog(PSL_WARNING, "Unable to verify: '%s'\n", gpg_strerror(tErr));
  }
  else if (NULL != (pResult = gpgme_op_verify_result(m_pGpgmeCtx)))
  {
    ps_elog(PSL_ERROR, "Unable to get result object.\n");
  }
  else if (NULL == pResult->signatures)
  {
    ps_elog(PSL_ERROR, "Unable to get result signature object.\n");
  }
  else if (GPG_ERR_NO_ERROR != pResult->signatures->status)
  {
    ps_elog(PSL_ERROR, "Signature was invalid: %s\n", gpg_strerror(pResult->signatures->status));
  }
  else
  {
    ps_elog(PSL_DEBUG, "Valid signature.\n");
    bRet = true;
  }

  if (NULL != pData)
  {
    gpgme_data_release(pData);
  }
  if (NULL != pSig)
  {
    gpgme_data_release(pSig);
  }

  return bRet;
}

bool GpgmeCryptMgr::verify(std::string &p_sSig, std::string &p_sData)
{
  bool bRet = false;

  ssize_t lRet = 0;
  size_t uSigSize = p_sSig.size();
  size_t uDataSize = p_sData.size();
  gpgme_error_t tErr;
  gpgme_data_t pData = NULL;
  gpgme_data_t pSig = NULL;
  gpgme_verify_result_t pResult = NULL;

  if (GPG_ERR_NO_ERROR != (tErr = gpgme_data_new(&pData)))
  {
    ps_elog(PSL_CRITICAL, "Unable to create data buffer: '%s'\n", gpgme_strerror(tErr));
  }
  else if (GPG_ERR_NO_ERROR != (tErr = gpgme_data_new(&pSig)))
  {
    ps_elog(PSL_CRITICAL, "Unable to create signature buffer: '%s'\n", gpgme_strerror(tErr));
  }
  else if (-1 == (lRet = gpgme_data_write(pSig, p_sSig.c_str(), uSigSize))
           || uSigSize != (size_t) lRet
           || 0 != (lRet = gpgme_data_seek(pSig, 0, SEEK_SET)))
  {
    ps_elog(PSL_CRITICAL, "Unable to write sig data: %d: %s\n", lRet, strerror(errno));
  }
  else if (-1 == (lRet = gpgme_data_write(pData, p_sData.c_str(), uDataSize))
           || uDataSize != (size_t) lRet
           || 0 != (lRet = gpgme_data_seek(pData, 0, SEEK_SET)))
  {
    ps_elog(PSL_CRITICAL, "Unable to write data data: %d: %s\n", lRet, strerror(errno));
  }
  else if (GPG_ERR_NO_ERROR != (tErr = gpgme_op_verify(m_pGpgmeCtx, pSig, pData, NULL)))
  {
    ps_elog(PSL_WARNING, "Unable to verify: '%s'\n'%s'\n!>'%s'\n",
            gpg_strerror(tErr),
            p_sSig.c_str(),
            p_sData.c_str());
  }
  else if (NULL == (pResult = gpgme_op_verify_result(m_pGpgmeCtx)))
  {
    ps_elog(PSL_ERROR, "Unable to get result object.\n");
  }
  else if (NULL == pResult->signatures)
  {
    ps_elog(PSL_ERROR, "Unable to get result signature object.\n");
  }
  else if (GPG_ERR_NO_ERROR != pResult->signatures->status)
  {
    ps_elog(PSL_ERROR, "Signature was invalid: %s\n", gpg_strerror(pResult->signatures->status));
  }
  else
  {
    ps_elog(PSL_DEBUG, "Valid signature.\n");
    bRet = true;
  }

  if (NULL != pData)
  {
    gpgme_data_release(pData);
  }
  if (NULL != pSig)
  {
    gpgme_data_release(pSig);
  }

  return bRet;
}

std::string &GpgmeCryptMgr::getSigningKey()
{
  return m_sCurrentSigningKey;
}

bool GpgmeCryptMgr::setSigningKey(std::string &p_sKey)
{
  bool bRet = false;

  gpgme_error_t tErr;
  gpgme_key_t tKey;
  tErr = gpgme_get_key(m_pGpgmeCtx, p_sKey.c_str(), &tKey, 1);

  if (0 != tErr || NULL == tKey)
  {
    ps_elog(PSL_CRITICAL, "Error while getting key with ID: '%s' as UID: %d: [%d] %s\n", p_sKey.c_str(), getuid(), tErr, gpgme_strerror(tErr));
  }
  else if (0 != (tErr = gpgme_signers_add(m_pGpgmeCtx, tKey)))
  {
    ps_elog(PSL_CRITICAL, "Error while adding signor key for ID: '%s': [%d] %s\n", p_sKey.c_str(), tErr, gpgme_strerror(tErr));
    gpgme_key_unref(tKey);
  }
  else
  {
    m_sCurrentSigningKey = p_sKey;
    gpgme_set_armor(m_pGpgmeCtx, 1);
    gpgme_set_passphrase_cb(m_pGpgmeCtx, bb_cb, (void *) m_sPP.c_str());
    gpgme_key_unref(tKey);
    bRet = true;
  }

  return bRet;
}

bool GpgmeCryptMgr::addPubKey(std::string &p_sKey)
{
  bool bRet = false;

  ssize_t lRet = 0;
  gpgme_error_t tErr;
  gpgme_data_t tKeyData;

  if (GPG_ERR_NO_ERROR != (tErr = gpgme_data_new(&tKeyData)))
  {
    ps_elog(PSL_ERROR, "Unable to create new data buffer: '%s'\n", gpgme_strerror(tErr));
  }
  else if (-1 == (lRet = gpgme_data_write(tKeyData, p_sKey.c_str(), p_sKey.size()))
           || p_sKey.size() != (size_t) lRet
           || 0 != (lRet = gpgme_data_seek(tKeyData, 0, SEEK_SET))
          )
  {
    gpgme_data_release(tKeyData);
    ps_elog(PSL_ERROR, "Unable to write key data: %d: %s\n", lRet, strerror(errno));
  }
  else if (GPG_ERR_NO_ERROR != (tErr = gpgme_op_import(m_pGpgmeCtx, tKeyData)))
  {
    gpgme_data_release(tKeyData);
    ps_elog(PSL_ERROR, "Unable to import key: '%s'\n", gpgme_strerror(tErr));
  }
  else
  {
    gpgme_data_release(tKeyData);
    ps_elog(PSL_INFO, "Loaded friend key: '%s'\n", p_sKey.c_str());
    bRet = true;
  }

  return bRet;
}

bool GpgmeCryptMgr::sign(std::string &p_sData, 
                         std::string &p_sOutputSig, 
                         bool p_bDetach)
{
  bool bRet = true;

  ps_elog(PSL_DEBUG, "SIGNING: '%s'\n", p_sData.c_str());
  if (bRet)
  {
    ssize_t lRet = 0;
    size_t uDataSize = p_sData.size();
    off_t tCurr = 0;
    gpgme_error_t tErr;
    gpgme_data_t tData;
    gpgme_data_t tSig;
    gpgme_sig_mode_t tMode = (p_bDetach) ? GPGME_SIG_MODE_DETACH : GPGME_SIG_MODE_CLEAR;
    if (GPG_ERR_NO_ERROR != (tErr = gpgme_data_new(&tData)))
    {
      ps_elog(PSL_CRITICAL, "Unable to create data buffer: '%s'\n", gpgme_strerror(tErr));
      bRet = false;
    }
    else if (GPG_ERR_NO_ERROR != (tErr = gpgme_data_new(&tSig)))
    {
      ps_elog(PSL_CRITICAL, "Unable to create signature buffer: '%s'\n", gpgme_strerror(tErr));
      gpgme_data_release(tData);
      bRet = false;
    }
    else if (-1 == (lRet = gpgme_data_write(tData, p_sData.c_str(), uDataSize))
             || uDataSize != (size_t) lRet
             || 0 != (lRet = gpgme_data_seek(tData, 0, SEEK_SET)))
    {
      ps_elog(PSL_CRITICAL, "Unable to write data: %d: %s\n", lRet, strerror(errno));
      gpgme_data_release(tData);
      gpgme_data_release(tSig);
      bRet = false;
    }
    else if (GPG_ERR_NO_ERROR != (tErr = gpgme_op_sign(m_pGpgmeCtx, tData, tSig, tMode)))
    {
      ps_elog(PSL_CRITICAL, "Unable to sign: '%s'\n", gpg_strerror(tErr));
      gpgme_data_release(tData);
      gpgme_data_release(tSig);
      bRet = false;
    }
    else if ((tCurr = gpgme_data_seek(tSig, 0, SEEK_CUR)) <= 0)
    {
      ps_elog(PSL_CRITICAL, "Unable to get sig's current offset: '%s'\n", strerror(errno));
      gpgme_data_release(tData);
      gpgme_data_release(tSig);
      bRet = false;
    }
    else
    {
      char *pBuff = new char[tCurr + 1];
      memset(pBuff, 0, tCurr + 1);
      gpgme_data_seek(tSig, 0, SEEK_SET);

      int iErr = gpgme_data_read(tSig, pBuff, tCurr);
      if (iErr <= 0)
      {
        ps_elog(PSL_CRITICAL, "Unable to read new signature: '%s'\n", strerror(errno));
        bRet = false;
      }
      else
      {
        p_sOutputSig = pBuff;
        bRet = true;
      }

      gpgme_data_seek(tData, 0, SEEK_SET);
      gpgme_data_seek(tSig, 0, SEEK_SET);
      gpgme_data_release(tData);
      gpgme_data_release(tSig);
      delete[] pBuff;
    }
  }

  return bRet;
}


GpgmeCryptMgr *GpgmeCryptMgr::dup()
{
  GpgmeCryptMgr *pRet = new GpgmeCryptMgr();
  if (!pRet->init())
  {
    ps_elog(PSL_CRITICAL, "Unable to init GPGME.\n");
    delete pRet;
    pRet = NULL;
  }

  return pRet;
}

