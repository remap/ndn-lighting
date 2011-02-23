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
#include "ps_logger.h"
#include <stdio.h>

#include "ps_thrd_pool.h"
#include "ps_thrd.h"
#include "ps_task.h"
#include "ps_app.h"
#include "ps_defs.h"

class HttpListenerCtx;

class TestApp : public PsApp
{
  public:
    virtual const char *getHttpPath(){ return "BLAH";};
    virtual const char *getHttpPass(){ return "BLAH";};
    virtual HttpListenerCtx *createCtx(){ return NULL;};
    virtual bool enabled(){return true;};
    virtual bool execute(){ return true;};
    virtual bool init(){ return true;};
};

class TestTask : public PsTask
{
  private:
    bool m_bDone;
    int m_iID;
    
  public:
    TestTask(PsApp &p_oApp, int p_iID);
    virtual ~TestTask();

    virtual bool execute();
    virtual bool done();
    virtual bool process();
};

TestTask::TestTask(PsApp &p_oApp, int p_iID)
  : m_bDone(false),
    m_iID(p_iID)
{
}

TestTask::~TestTask()
{
}

bool TestTask::execute()
{
  for (int i = 0; i < 100; i++)
  {
    printf("%d ", i);
  }
  printf("\n");
  m_bDone = true;

  return true;
}

bool TestTask::done()
{
  return m_bDone;
}

bool TestTask::process()
{
  bool bRet = false;
  if (done())
  {
    printf("Done w/ %d\n", m_iID);
    bRet = true;
  }
  else
  {
    printf("NOT DONE: %d\n", m_iID);
  }
  return bRet;
}

int main(int argc, char *argv[])
{
  int iRet = 1;

  PsThreadPool oPool;
  oPool.setSize(10);
  printf("Initializing pool...\n");
  if (!oPool.init())
  {
    ps_elog(PSL_CRITICAL, "Could not init pool.\n");
  }
  else
  {
    TestApp oApp;
    printf("Adding tasks...\n");
    PsTaskList_t oList;
    for (int i = 0; i < 20; i++)
    {
      TestTask *pTask = new TestTask(oApp, i);
      oList.push_back(pTask);
      PsThread *pThrd = oPool.checkOut();
      pThrd->setTask(pTask);
      printf("\tAdded %d\n", i);
    }

    printf("Waiting for pool to finish");
    while (oPool.getNumFree() < oPool.getSize())
    {
      printf(".");
      sleep(1);
    }
    printf("\n");
    printf("Killing pool.\n");
    oPool.kill();

    for (PsTaskIter_t tIter = oList.begin();
         oList.end() != tIter;
         tIter++)
    {
      delete (*tIter);
    }

    printf("Done!\n");
    iRet = 0;
  }

  return iRet;
}
