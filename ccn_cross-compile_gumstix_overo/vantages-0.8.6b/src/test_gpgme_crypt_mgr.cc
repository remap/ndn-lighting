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
#ifdef HAVE_READPASSPHRASE_H
#include <readpassphrase.h>
#elif defined(HAVE_PWD_H)
#include <pwd.h>
#endif
#include <string.h>
#include <errno.h>

#include <string>

#include "ps_crypt_fact.h"
#include "ps_logger.h"
#include "gpgme_crypt_mgr.h"
/*
#include "dao_factory.h"
#include "dns_rr_fact.h"
*/

using namespace std;

int main(int argc, char *argv[])
{
  int iRet = 0;

  PsLogger::getInstance().setLevel(PSL_INFO);
  if (argc < 3)
  {
    ps_elog(PSL_CRITICAL, "Unable to run w/o key ID and something to encrypt.\n");
  }
  else
  {
    string sKeyID = argv[1];
    string sData = argv[2];
    string sSig;


#ifdef HAVE_READPASSPHRASE_H
    char sz[1024];
    memset(sz, 0, 1024);
    readpassphrase("PP> ", sz, 1024, 0);
    string sPP = sz;
#else
    char *sz = getpass("PP> ");
    string sPP = (NULL == sz) ? "" : sz;
#endif
    sPP += "\n";
    close(STDIN_FILENO);

    for (int i = 0; i < 5; i++)
    {
      GpgmeCryptMgr *pMgr = (GpgmeCryptMgr *) PsCryptFactory::getInstance().create(GpgmeCryptMgr::GPGMR_CRYPT_MGR);
//    GpgmeCryptMgr *pMgr = new GpgmeCryptMgr();

      for (int j = 0; j < 5; j++)
      {
        bool bSigned = false;

        if (NULL == pMgr)
        {
          ps_elog(PSL_CRITICAL, "Unable to create manager named: '%s'\n", GpgmeCryptMgr::GPGMR_CRYPT_MGR);
        }
/*
else if (!pMgr->init())
{
ps_elog(PSL_CRITICAL, "Unable to init mgr.\n");
}
*/
/*
        else if (sPP == "\n")
        {
          ps_elog(PSL_CRITICAL, "Unable to get passphrase: '%s'\n", strerror(errno));
        }
*/
        else if (!pMgr->setPP(sPP))
        {
          ps_elog(PSL_CRITICAL, "Unable to set PP.\n");
        }
        else if (!pMgr->setSigningKey(sKeyID))
        {
          ps_elog(PSL_CRITICAL, "Unable to set signing key: '%s'\n", sKeyID.c_str());
        }
        else if (!pMgr->sign(sData, sSig, true))
        {
          ps_elog(PSL_CRITICAL, "Unable to sign.\n");
        }
        else if (!pMgr->sign(sData, sSig, true))
        {
          ps_elog(PSL_CRITICAL, "Unable to sign 2nd time.\n");
        }
        else
        {
          fprintf(stderr, "SIGNED using key '%s'::: '%s' -> '%s'\n", sKeyID.c_str(), sData.c_str(), sSig.c_str());
          bSigned = true;
        }

        if (bSigned)
        {
          if (!pMgr->verify(sSig, sData))
          {
            ps_elog(PSL_CRITICAL, "Unable to verify signature: '%s'\non data: '%s'\n", sSig.c_str(), sData.c_str());
          }
          else
          {
            fprintf(stderr, ">>>VERIFIED!!!<<<\n");
          }
        }
      }

      if (NULL != pMgr)
      {
        delete pMgr;
        pMgr = NULL;
      }
    }
  }

  PsCryptFactory::getInstance().reset();
  sleep(1);

  return iRet;
}
