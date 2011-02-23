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

#include "ps_thrd.h"
#include "ps_thrd_pool.h"
#include "ps_task.h"
#include "ps_logger.h"
#include "ps_defs.h"

using namespace std;

void *_run(void *p_pParam)
{
  if (NULL == p_pParam)
  {
    ps_elog(PSL_CRITICAL, "Callback got NULL paramerer.\n");
  }
  else
  {
    ((PsThread *) p_pParam)->run();
    ps_elog(PSL_CRITICAL, "Thread done.\n");
  }

  return NULL;
}

PsThread::PsThread(PsThreadPool *p_pPool /*= NULL*/)
  : m_bRun(false),
    m_pTask(NULL),
    m_pPool(p_pPool)
{

}

PsThread::~PsThread()
{

}

bool PsThread::start()
{
  bool bRet = false;

  int iErr = 0;
  if (m_bRun)
  {
    ps_elog(PSL_CRITICAL, "Thread already running.\n");
  }
  else
  {
    m_bRun = true;
    if (0 != (iErr = pthread_create(&m_tID, NULL, _run, this)))
    {
      ps_elog(PSL_CRITICAL, "Unable to start thread: %s\n", strerror(iErr));
    }
    else
    {
      bRet = true;
    }
  }

  return bRet;
}

bool PsThread::run()
{
  bool bRet = false;

  while (m_bRun)
  {
    bRet = true;

    PsTask *pTask = getTask();
    if (NULL == pTask)
    {
      sleep(1);
    }
    else
    {
      if (!pTask->execute())
      {
        ps_elog(PSL_CRITICAL, "Unable to execute task.\n");
      }

      setTask(NULL);

      if (NULL != m_pPool && !m_pPool->checkIn(*this))
      {
        ps_elog(PSL_CRITICAL, "Unable to check this thread in\n");
      }
    }
  }

  return bRet;
}

bool PsThread::kill()
{
  m_bRun = false;

  return !m_bRun;
}

bool PsThread::join()
{
  int iErr = pthread_join(m_tID, NULL);
  if (0 != iErr)
  {
    ps_elog(PSL_CRITICAL, "Unable to join thread: %s\n", strerror(iErr));
  }

  return (0 == iErr);
}

bool PsThread::getRun()
{
  return m_bRun;
}

PsTask *PsThread::getTask()
{
  return m_pTask;
}

void PsThread::setTask(PsTask *p_pTask)
{
  m_pTask = p_pTask;
}

PsThreadPool *PsThread::getPool()
{
  return m_pPool;
}

void PsThread::setPool(PsThreadPool *p_pPool)
{
  m_pPool = p_pPool;
}

PsThread *PsThread::dup()
{
  return new PsThread(m_pPool);
}
