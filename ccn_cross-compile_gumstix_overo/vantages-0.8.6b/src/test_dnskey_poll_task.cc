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
#include <stdlib.h>

#include <string>
#include <list>

#include "ps_poller.h"
#include "ps_thrd.h"
#include "ps_logger.h"
#include "http_list_ctx.h"
#include "ps_app.h"
#include "dnskey_poll_task.h"
#include "ps_defs.h"
#include "dns_resolver.h"

using namespace std;

class TestApp : public PsApp
{
  public:
    TestApp() { };
    virtual ~TestApp() { };

   virtual const char *getHttpPath(){ return "BLAH";};
   virtual const char *getHttpPass(){ return "BLAH";};
   virtual HttpListenerCtx *createCtx(){ return NULL;};
   virtual bool enabled(){return true;};
   virtual bool execute(){ return true;};
   virtual bool init(){ return true;};
};


class TestThr : public PsThread
{
  private:
    int m_iRuns;

  public:
    TestThr(int p_iRuns) : m_iRuns(p_iRuns){ };
    virtual ~TestThr(){ };

    virtual bool run();
};

bool TestThr::run()
{
  TestApp oApp;
  PsPoller oPoller;
  oPoller.init(20, 1);
  PsPollTaskList_t oList;

  for (int j = 0; j < 20; j++)
  {
    DnskeyPollTask *pTask = new DnskeyPollTask(oApp);
    string sSrc = "dnssec:isc.org.?type=48";
    string sName = "isc.org.";
    pTask->init(sSrc, sName, "BLAH", "BLAH", 2);

    oList.push_back(pTask);
  }

  for (int i = 0; i < m_iRuns; i++)
  {
    if (!oPoller.poll(oList.begin(), oList.end()))
    {
      ps_elog(PSL_CRITICAL, "Unable to poll?\n");
    }
  }

  for (PsPollTaskIter_t tIter = oList.begin();
       oList.end() != tIter;
       tIter++)
  {
    delete *tIter;
  }

  return true;
}

int main(int argc, char *argv[])
{
  int iRet = 0;

  int iConc = 1;
  int iRuns = 10;
  if (argc > 1)
  {
    iConc = (int) strtol(argv[1], NULL, 10);
  }

  if (argc > 2)
  {
    iRuns = (int) strtol(argv[2], NULL, 10);
  }

  DnsResolver oRes;

  list<TestThr *> oThrList;
  for (int i = 0; i < iConc; i++)
  {
    TestThr *pThr = new TestThr(iRuns);
    if (!pThr->start())
    {
      ps_elog(PSL_CRITICAL, "Unable to start thread.\n");
      delete pThr;
    }
    else
    {
      oThrList.push_back(pThr);
    }
  }

  for (list<TestThr *>::iterator tIter = oThrList.begin();
       oThrList.end() != tIter;
       tIter++)
  {
    TestThr *pThr = *tIter;
    pThr->join();
    delete pThr;
  }

  return iRet;
}
