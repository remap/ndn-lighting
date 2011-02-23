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
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <string>

#include "dsync_app.h"
#include "ps_config.h"
#include "ps_logger.h"

#include "dns_err.h"

using namespace std;

RRList_t parseKeyfile(char* filename);
const char *DsyncApp::s_szHttpPath = "/dsync";

DsyncApp::DsyncApp()
{

}

DsyncApp::~DsyncApp()
{

}

bool DsyncApp::enabled()
{
  return (NULL != PsConfig::getInstance().getValue(DSYNC_CONFIG_APP_ENABLED));
}

bool DsyncApp::init()
{
  bool bRet = false;
  ps_elog(PSL_DEBUG, "DsyncApp Initializing...\n");
  const char *szResConc = PsConfig::getInstance().getValue(PS_CONFIG_RES_CONC);
  if (NULL != szResConc)
  {
    int iConc = atoi(szResConc);
    ps_elog(PSL_INFO, "Setting resolver concurrency to %d\n", iConc);
    oRes.setConcurrency(iConc);
  }

  const char *szWeighted = PsConfig::getInstance().getValue(DSYNC_WEIGHTED_CHECK);
  if (NULL != szWeighted)
  {
    weighted_check = atoi(szWeighted);
  }
  else
  {
    ps_elog(PSL_WARNING, "No resolver concurrency secified in config file, so using default.\n");
  }

  oRes.setDO(true);
  oRes.setBuffSize(4096);
  bRet = true;
  return bRet;
}

bool DsyncApp::execute()
{
  //ps_elog(PSL_DEBUG, "DsyncApp: starting execution\n");
  bool bRet = true;
  
  while (getRun())
  {
    //ps_elog(PSL_DEBUG, "dsync: waking up\n");
    
    DsyncZoneDao *zDao = new DsyncZoneDao();
    DaoList_t zones;
    zDao->deserialize(zones);
    //printf("deserialized %d zones\n", (int)zones.size());
    
    for (DaoIter_t it = zones.begin();
         it != zones.end();
         it++)
    { 

      DsyncZoneDao* pDao = (DsyncZoneDao*) *it;
      //the cases where we want to generate an alert, before moving into routine execution
      if (pDao->getSyncState() == 10)
      {
        ps_elog(PSL_INFO, "Notification: Zone %s is now back in-sync\n", pDao->zoneName.c_str());
        ps_syslog("Notification: Zone %s is now back in-sync", pDao->zoneName.c_str());
        pDao->setSyncState(0);
      }
      else if (pDao->getSyncState() == 11)
      {
        ps_elog(PSL_INFO, "Notification: Zone %s should now update DS record for %s\n",
                pDao->parentName.c_str(), pDao->zoneName.c_str());
        ps_syslog("Notification: Zone %s should now update DS record for %s",
                  pDao->parentName.c_str(), pDao->zoneName.c_str());
        pDao->setSyncState(1);
      }
      else if (pDao->getSyncState() == 12)
      {
        ps_elog(PSL_INFO, "Notification: Zone %s should now delete old key\n", pDao->zoneName.c_str());
        ps_syslog("Notificaiton: Zone %s should now delete old key", pDao->zoneName.c_str());
        pDao->setSyncState(2);
      }
      

      DsyncDnskeyAbstractionTask* tTask = NULL;
      if (pDao->getSyncState() == 0)
      { 
        //printf("Got new task for syncState 0\n");
        tTask = new DsyncFirstTask(pDao, weighted_check);
      }
      else if (pDao->getSyncState() == 1)
      {
        //printf("Got new task for syncState 1\n");
        tTask = new DsyncSecondTask(pDao);
      }
      else if (pDao->getSyncState() == 2)
      {
        //printf("Got new task for syncState 2\n");
        tTask = new DsyncThirdTask(pDao);
      }


      if (tTask != NULL)
      {
        tTask->execute();
      }
      oRes.setDO(true);
      if (!oRes.send(tTask))
      {
        ps_elog(PSL_CRITICAL, "Couldn't send because: '%s'\n", DnsError::getInstance().getError().c_str());
      }
    }


    while (oRes.hasTasks())
    {
      DsyncDnskeyAbstractionTask* pTask = NULL;
      /*
      for (int i = 0;
           i < oRes.getConcurrency()
           && oRes.hasTasks()
           //&& NULL != (pTask = (DsyncDnskeyAbstractionTask*)oRes.recv());
            ;
           i++)
      */
      {
        
        if (NULL == (pTask = (DsyncDnskeyAbstractionTask*)oRes.recv()))
        {
          continue;
        }
        else if (pTask->getTaskType() == "init")
        {
          pTask = (DsyncInitTask*)pTask;
        }
        else if (pTask->getTaskType() == "first")
        {
          pTask = (DsyncFirstTask*)pTask;
        }
        else if (pTask->getTaskType() == "second")
        {
          pTask = (DsyncSecondTask*)pTask;
        }
        else if (pTask->getTaskType() == "third")
        {
          pTask = (DsyncThirdTask*)pTask;
        }
        else
        {
          ps_elog(PSL_CRITICAL, "Unknown task type, casting to firstTask for funnnnzies\n");
          pTask = (DsyncFirstTask*)pTask;
        }
        bRet = true;
        if (!pTask->done())
        {
          pTask->process();
          pTask->execute();
          oRes.setDO(true);
          if (!oRes.send((DnsTask*)pTask))
          {
            ps_elog(PSL_INFO, "Unable to re-send task... Dropping\n");
          }
        }
        else
        {
          delete pTask;
        }

        pTask = NULL;

      }

    }

    sleep(10);
    delete zDao;
  }
  return bRet;
}

const char * DsyncApp::getHttpPath()
{
  return s_szHttpPath;
}

const char * DsyncApp::getHttpPass()
{
  return PsConfig::getInstance().getValue(DSYNC_CONFIG_ADMIN_PASS);
}

HttpListenerCtx * DsyncApp::createCtx()
{
  return new DsyncCtx();
}

RRList_t parseKeyfile(char* filename)
{
  RRList_t keyList;
  ifstream inFile;
  inFile.open(filename);
  if (!inFile)
  {
    ps_elog(PSL_CRITICAL, "Unable to open key file: %s\n");
    exit(1);
  }

  string kName;
  string kClass;
  string kType;
  string kFlags;
  string kAlg;
  string kProto;
  string kKey;
  vector<u_char> binKey;

  inFile >> kName;
  inFile >> kClass;
  inFile >> kType;
  inFile >> kFlags;
  inFile >> kProto;
  inFile >> kAlg;
  inFile >> kKey; 
  binKey = base64_decode(kKey);
  u_char bArr [binKey.size()];
  int i = 0;
  for (vector<u_char>::iterator it = binKey.begin();
        it != binKey.end();
        it++)
  {
    bArr[i++] = *it;
  }

  DnsDnskey* key = new DnsDnskey();
  DnsName name(kName);
  key->set_name(name);
  if (kClass != "IN")
  {
    ps_elog(PSL_ERROR, "Class was not the expected DNS_CLASS_IN\n");
  } 
  else
  {
    key->set_class(DNS_CLASS_IN);
  }
  key->set_class(1);
  key->setFlags((uint16_t)atoi(kFlags.c_str()));
  key->setAlgo((uint8_t)atoi(kAlg.c_str()));
  key->setProto((uint8_t)atoi(kProto.c_str()));
  key->setKey(kKey);
  key->setBinKey(bArr, binKey.size());

  key->print();
  
  keyList.push_back(key);

  inFile.close();
  return keyList;  
}
