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

#include "config.h"

#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#ifdef HAVE_READPASSPHRASE_H
#include <readpassphrase.h>
#endif
#if defined(HAVE_PWD_H)
#include <pwd.h>
#endif

#include <vector>

#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>

#include "ps_poller.h"
#include "ps_defs.h"
#include "ps_config.h"
#include "dao_factory.h"
#include "dns_rr_fact.h"
#include "http_listener.h"
#include "friend_rule.h"
#include "bind_ta_gen.h"
#include "dnskey_app.h"
#include "dnskey_admin_ctx.h"
#include "ps_local_key_dao.h"
#include "gpgme_crypt_mgr.h"
#include "ps_crypt_fact.h"
#include "ps_thrd.h"
#include "ps_thrd_pool.h"
#include "ps_logger.h"
#include "ps_util.h"
#include "dsync_app.h"

using namespace std;

bool g_bRun = true;
bool g_bReloadConf = false;

bool _loggingSetup(bool p_bFG)
{
  bool bRet = true;

  PsConfig &oConfig = PsConfig::getInstance();
  const char *szConfLogFile = oConfig.getValue(PS_CONF_LOG_FILE);
  if (!p_bFG
      && NULL != szConfLogFile
      && PsLogger::getInstance().getFileName() == "")
  {
    if (!(PsLogger::getInstance().setFileName(szConfLogFile)))
    {
      ps_elog(PSL_CRITICAL, "Unable to set file '%s' as output file, using stderr\n", szConfLogFile);
      bRet = false;
    }
  }

  const char *szConfLogLevel = oConfig.getValue(PS_CONF_LOG_LEVEL);
  if (NULL != szConfLogLevel)
  {
    string sLevel = szConfLogLevel;
    int iLogLevel = PsLogger::getInstance().strToLevel(sLevel);
    if (-1 == iLogLevel)
    {
      ps_elog(PSL_CRITICAL, "Unable to set log level '%s', using stderr\n", szConfLogLevel);
      bRet = false;
    }
    else
    {
      PsLogger::getInstance().setLevel(iLogLevel);
    }
  }

  return bRet;
}

void *_spawnHttpd(void *p_pListener)
{
  HttpListener *pListener = (HttpListener *) p_pListener;
  if (NULL == pListener)
  {
    ps_elog(PSL_CRITICAL, "Listener cannot be NULL.\n");
  }
  else
  {
    pListener->listen();
  }

  return NULL;
}

void _version()
{
//  fprintf(stdout, "\nvantaged 0.8.0 Beta (tools@netsec.colostate.edu)\n\n");
  fprintf(stdout, "\nvantaged %s (tools@netsec.colostate.edu)\n\n", PACKAGE_VERSION);
}

void _usage()
{
  _version();
  printf("vantaged [ OPTIONS ]\n");
  printf("\n");
  printf("OPTIONS:\n");
  printf("      -c <config file>          This file overrides the default location\n");
  printf("                                of the config file (/etc/vantaged.conf)\n");
  printf("      -r <runs>                 The number of loops before the daemon will\n");
  printf("                                exit.\n");
  printf("      -p <pid file>             a file to contain the PID of the daemon process.\n");
  printf("      -o <log file>             a file to contain the output of the daemon process.\n");
  printf("      -g                        Run daemon in the foreground.\n");
  printf("      -u <user>                 Drop permissions to this user after init.\n");
  printf("      -l 'DEBUG'                The output message logging level\n");
  printf("         | 'INFO'\n");
  printf("         | 'WARNING'\n");
  printf("         | 'ERROR'\n");
  printf("         | 'CRITICAL'\n");
  printf("      -v                        Version information.\n");
  printf("      -h                        This message.\n");
}

void _sighupHandler(int p_iSig)
{
  g_bReloadConf = true;
}

static int _daemonize()
{
  int iRet = -1;
  pid_t tPid = -1;

  //fprintf(stderr, "Daemonizing...\n");
  if ((tPid = fork()) < 0)
  {
    ps_elog(PSL_CRITICAL, "Unable to fork: %s\n", strerror(errno));
  }
  else if (tPid > 0)
  {
    iRet = tPid;
  }
  else if (setsid() < 0)
  {
    ps_elog(PSL_CRITICAL, "Unable to create new process group: %s\n", strerror(errno));
  }
  else
  {
    close(0);
    close(1);
    close(2);
    int iFD = open("/dev/null", O_RDWR);
    if (iFD < 0)
    {
      ps_elog(PSL_CRITICAL, "Unable to open /dev/null: %s\n", strerror(errno));
    }
    else
    {
      int iRetVal = dup(iFD);
      iRetVal = dup(iFD);
      iRetVal = 0;

      if (chdir("/") < 0)
      {
        ps_elog(PSL_CRITICAL, "Unable to chdir to /: %s\n", strerror(errno));
      }
      else
      {
        iRet = 0;
      }
    }
  }

  return iRet;
}

bool _getPP(std::string &p_sPP, std::string &p_sKeyName)
{
  bool bRet = false;

  PsLocalKeyDao *pSigningKey = dynamic_cast<PsLocalKeyDao *>(DaoFactory::getInstance().create(PsLocalKeyDao::s_kszDaoName));
  if (NULL == pSigningKey)
  {
    ps_elog(PSL_CRITICAL, "Unable to create local key DAO... No signatures can be made or checked.\n");
  }
  else if (!pSigningKey->deserialize())
  {
    ps_elog(PSL_CRITICAL, "No signing key configured, no signtures can be made.  Try running:\n\tvant-setup\n");
    delete pSigningKey;
  }
  else
  {
    p_sPP = pSigningKey->getPhrase();
    if (p_sPP == "")
    {
#ifdef HAVE_READPASSPHRASE_H
      char sz[1024];
      memset(sz, 0, 1024);
      readpassphrase("Input GPG Key Passphrase> ", sz, 1024, 0);
      p_sPP = sz;
#else
      char *sz = getpass("Input GPG Key Passphrase> ");
      p_sPP = (NULL == sz) ? "" : sz;
#endif
    }

    p_sPP += "\n";
    p_sKeyName = pSigningKey->getKeyName();

    delete pSigningKey;
    pSigningKey = NULL;
//ps_elog(PSL_CRITICAL, "PP: '%s'\n", sPP.c_str());
    bRet = true;
  }

  return bRet;
}

bool _cryptPpCheck(std::string &p_sPP, std::string &p_sKeyName, std::string &p_szHomeDir)
{
  bool bRet = false;

  string sBlahName = GpgmeCryptMgr::GPGMR_CRYPT_MGR;
  ps_elog(PSL_DEBUG, "Verifying passphrase.\n");
  GpgmeCryptMgr *pMgr = NULL;

  if (p_sPP == "")
  {
    ps_elog(PSL_CRITICAL, "No passphrase specified.\n");
  }
  else if (NULL == (pMgr = static_cast<GpgmeCryptMgr *>(PsCryptFactory::getInstance().create(sBlahName))))
  {
    ps_elog(PSL_CRITICAL, "Unable to create crypto mgr for name '%s'\n", sBlahName.c_str());
    bRet = false;
  }
  else
  {
    string sData = "test string";
    string sSig;

    pMgr->setPP(p_sPP);
    pMgr->setHomeDir(p_szHomeDir.c_str());

    if (!pMgr->init())
    {
      ps_elog(PSL_CRITICAL, "Unable to init crypto Manager.\n");
    }
    else if (!pMgr->setSigningKey(p_sKeyName))
    {
      ps_elog(PSL_CRITICAL, "Unable to set signing key with name: '%s'\n", p_sKeyName.c_str());
    }
    else if (!pMgr->sign(sData, sSig, true))
    {
      ps_elog(PSL_CRITICAL, "Unable to create signature: '%s'\non data: '%s'\n", sSig.c_str(), sData.c_str());
    }
    else if (!pMgr->verify(sSig, sData))
    {
      ps_elog(PSL_CRITICAL, "Unable to verify signature: '%s'\non data: '%s'\n", sSig.c_str(), sData.c_str());
    }
    else
    {
      ps_elog(PSL_INFO, "Passphrase verified.\n");
      bRet = true;
    }

    delete pMgr;
  }

  return bRet;
}

bool _cryptInit(AppList_t &p_oAppList, std::string &p_sPP, std::string &p_sKeyName, std::string &p_szHomeDir)
{
  bool bRet = false;

  if (p_sPP == "")
  {
    ps_elog(PSL_CRITICAL, "No passphrase specified.\n");
  }
  else
  {
    string sBlahName = GpgmeCryptMgr::GPGMR_CRYPT_MGR;
    ps_elog(PSL_DEBUG, "Setting crypt mgr in DNSKEY app(s).\n");
    bRet = true;
    for (AppIter_t tIter = p_oAppList.begin();
         p_oAppList.end() != tIter;
         tIter++)
    {
      GpgmeCryptMgr *pMgr = dynamic_cast<GpgmeCryptMgr *>(PsCryptFactory::getInstance().create(sBlahName));
      if (NULL == pMgr)
      {
        ps_elog(PSL_CRITICAL, "Unable to create crypto mgr for name '%s'\n", sBlahName.c_str());
        bRet = false;
        break;
      }
      else
      {
        pMgr->setPP(p_sPP);
        pMgr->setHomeDir(p_szHomeDir.c_str());

        if (!pMgr->init())
        {
          ps_elog(PSL_CRITICAL, "Unable to init crypto Manager.\n");
          delete pMgr;
        }
        else if (!pMgr->setSigningKey(p_sKeyName))
        {
          ps_elog(PSL_CRITICAL, "Unable to set signing key with name: '%s'\n", p_sKeyName.c_str());
          delete pMgr;
          bRet = false;
          break;
        }
        else if (!pMgr->loadFriendKeys())
        {
          ps_elog(PSL_CRITICAL, "Unable to load friends in crypto mgr.\n");
          delete pMgr;
          bRet = false;
          break;
        }
        else
        {
          (*tIter)->setCryptMgr(pMgr);
        }
      }
    }
  }

  return bRet;
}

int main(int argc, char *argv[])
{
  int iRet = 1;
//  signal(SIGPIPE, SIG_IGN);
  signal(SIGHUP, _sighupHandler);

//  string sConfigFile = getenv("HOME");
//  sConfigFile += "/.vantaged.conf";
  string sConfigFile = "/etc/vantaged.conf";

  pthread_t tID = 0;
  int iTotalRuns = -1;
  int iErr = 0;
  bool bFG = false;
  const char *szPidFile = NULL;
  int iLogLevel = PSL_CRITICAL;
  string sLevel;
  struct passwd *pPasswd = NULL;
  char *szHomeDir = NULL;
  string p_szHomeDir;

  int c;
  while ((c = getopt(argc, argv, "c:r:p:o:gu:l:hv")) != -1)
  {
    switch (c)
    {
      case 'c':
        sConfigFile = optarg;
        break;
      case 'r':
        iTotalRuns = (int) strtol(optarg, NULL, 10);
        break;
      case 'p':
        szPidFile = optarg;
        break;
      case 'o':
        if (!(PsLogger::getInstance().setFileName(optarg)))
        {
          ps_elog(PSL_CRITICAL, "Unable to set file '%s' as output file, using stderr\n", optarg);
        }
        break;
      case 'u':
        if (getuid() != 0)
        {
          ps_elog(PSL_CRITICAL, "Unable to change user from non-root user.\n");
          exit(1);
        }
        else if (NULL == (pPasswd = getpwnam(optarg)))
        {
          ps_elog(PSL_CRITICAL, "Unable to locate user: '%s'...  Not dropping permissions.\n", optarg);
        }
        else
        {
          szHomeDir = pPasswd->pw_dir;
          p_szHomeDir = string (szHomeDir) + "/.gnupg";
          //printf ("p_szHomeDir = %s\n", p_szHomeDir.c_str());
        }

        break;
      case 'g':
        bFG = true;
        break;
      case 'h':
        _usage();
        exit(0);
        break;
      case 'v':
        _version();
        exit(0);
        break;
      case 'l':
        sLevel = optarg;
        iLogLevel = PsLogger::getInstance().strToLevel(sLevel);

        if (-1 == iLogLevel)
        {
          ps_elog(PSL_CRITICAL, "Unknown log level '%s', leaving at CRITICAL\n", sLevel.c_str());
          iLogLevel = PSL_CRITICAL;
        }
        break;
      case '?':
      default:
        ps_elog(PSL_CRITICAL, "Unable to parse command line.\n");
        _usage();
        exit(1);
        break;
    }
  }

  PsLogger::getInstance().setLevel(iLogLevel);
  PsConfig &oConfig = PsConfig::getInstance();
  if (!oConfig.load(sConfigFile.c_str()))
  {
    ps_elog(PSL_CRITICAL, "Unable to load config file '%s'.\n", sConfigFile.c_str());
  }
  else
  {
    if (!_loggingSetup(bFG))
    {
      ps_elog(PSL_CRITICAL, "Unable to initialize logging.\n");
    }

    AppList_t oAppList;

    // The DNSKEY TAR application
    DnskeyApp oDnskeyApp;
    DnskeyAdminCtx oDnskeyAdmin(oDnskeyApp);

    // The Dsync Application
    DsyncApp oDsyncApp;
  
    if (oDnskeyApp.enabled())
    {
      if (!oDnskeyApp.init())
      {
        ps_elog(PSL_CRITICAL, "Unable to init DNSKEY app, not running...\n");
      }
      if (!oDnskeyAdmin.init())
      {
        ps_elog(PSL_CRITICAL, "Unable to init DNSKEY admin app, not running DNSKEY App either...\n");
      }
      else
      {
        oAppList.push_back(&oDnskeyApp);
        oAppList.push_back(&oDnskeyAdmin);
      }
    }

    if (oDsyncApp.enabled())
    {
      if (!oDsyncApp.init())
      {
        ps_elog(PSL_CRITICAL, "Unable to init DSYNC app, not running...\n");
      }
      else
      {
        oAppList.push_back(&oDsyncApp);
      }
    }

    bool bValidPP = false;
    string sPP;
    string sKeyName;
    for (int iPpCheck = 0; iPpCheck < 3; iPpCheck++)
    {
      if (!_getPP(sPP, sKeyName))
      {
        ps_elog(PSL_CRITICAL, "Unable to start vantaged without crypo init, which needs a passphrase (even if it's empty).\n");
      }
      else if (!_cryptPpCheck(sPP, sKeyName, p_szHomeDir))
      {
        ps_elog(PSL_CRITICAL, "Invalid passphrase.\n");
      }
      else
      {
        bValidPP = true;
        break;
      }
    }

    if (!bValidPP)
    {
      ps_elog(PSL_CRITICAL, "Unable to validate passphrase, exiting...\n");
      iRet = 1;
    }
    else if (!bFG && (iErr = _daemonize()) < 0)
    {
      ps_elog(PSL_CRITICAL, "Unable to create daemon.\n");
      iRet = iErr;
    }
    else if (!bFG && iErr > 0)
    {
      PsLogger::getInstance().close();
      if (NULL == szPidFile)
      {
        szPidFile = oConfig.getValue(PS_CONFIG_PID_FILE);
      }

      FILE *pPidFile = fopen(szPidFile, "w");
      if (NULL == pPidFile)
      {
        fprintf(stderr, "Unable to create PID file: %s\n", strerror(errno));
      }
      else
      {
        fprintf(pPidFile, "%d", iErr);
        iRet = 0;
      }
    }
    else
    {
      if (NULL != pPasswd
               && setuid(pPasswd->pw_uid) < 0)
      {
        ps_elog(PSL_CRITICAL, "Unable to setuid to user ID: %d\n", pPasswd->pw_uid);
      }

    sleep(1);

      PsThreadPool oAppPool;
      oAppPool.setSize(oAppList.size());
      if (!_cryptInit(oAppList, sPP, sKeyName, p_szHomeDir))
      {
        printf ("After call to _cryptInit, p_szHomeDir = %s\n", p_szHomeDir.c_str());
        ps_elog(PSL_CRITICAL, "Unable to init crypto engine.\n");
      }
      else if (!oAppPool.init())
      {
        ps_elog(PSL_CRITICAL, "Unable to init Applications Pool.\n");
        oAppPool.kill();
        iTotalRuns = 1;
      }
      else
      {
        for (AppIter_t tAppIter = oAppList.begin();
             oAppList.end() != tAppIter;
             tAppIter++)
        {
          PsThread *pThrd = oAppPool.checkOut();
          if (NULL == pThrd)
          {
            ps_elog(PSL_CRITICAL, "Unable to get thread from App Pool...\n");
            oAppPool.kill();
            iTotalRuns = 1;
            break;
          }
          else
          {
            pThrd->setTask(*tAppIter);
          }
        }
      }


      HttpListener oAgent;
      char szDefaultPort[11];
      memset(szDefaultPort, 0, 11);
      sprintf(szDefaultPort, "%d", PS_DEFAULT_HTTP_PORT);
      const char *szPort = PsConfig::getInstance().getValue(PS_CONFIG_HTTP_PORT);
      if (NULL == szPort)
      {
        szPort = szDefaultPort;
      }

      if (!oAgent.init(szPort, oAppList))
      {
        ps_elog(PSL_CRITICAL, "Unable to init HTTP daemon with port: %s\n", szPort);
      }
      else if (0 != (iErr = pthread_create(&tID, NULL, _spawnHttpd, (void *) &oAgent)))
      {
        ps_elog(PSL_CRITICAL, "Unable to start HTTPD thread: %s\n", strerror(iErr));
      }
      else
      {
        curl_global_init(CURL_GLOBAL_ALL);
        int iRunCounter = 0;

        while (g_bRun)
        {
          if (g_bReloadConf)
          {
            ps_elog(PSL_CRITICAL, "Reloading config file...\n");
            if (!oConfig.load(sConfigFile.c_str()))
            {
              ps_elog(PSL_CRITICAL, "Unable to load config file '%s'.\n", sConfigFile.c_str());
            }
            if (!DnsResolver::init())
            {
              ps_elog(PSL_CRITICAL, "Unable init default resolver.\n");
            }

            if (!_loggingSetup(bFG))
            {
              ps_elog(PSL_CRITICAL, "Unable to initialize logging.\n");
            }

            DnsResolver oRes;
            ps_elog(PSL_INFO, "Default resolver is: %s\n", ps_inet_ntoa(oRes.getNameserver()).c_str());
            g_bReloadConf = false;
          }

          sleep(1);
          if (iTotalRuns > 0 && ++iRunCounter >= iTotalRuns)
          {
            oAgent.kill();
            for (AppIter_t tAppIter = oAppList.begin();
                 oAppList.end() != tAppIter;
                 tAppIter++)
            {
              (*tAppIter)->kill();
            }

            g_bRun = false;
          }
        }
      }
      pthread_join(tID, NULL);

      for (AppIter_t tAppIter = oAppList.begin();
           oAppList.end() != tAppIter;
           tAppIter++)
      {
        (*tAppIter)->kill();
      }
    }
    curl_global_cleanup();
    DaoFactory::getInstance().reset();
    DnsRrFactory::getInstance().reset();
    PsDao::finalize();
    iRet = 0;
  }

  return iRet;
}
