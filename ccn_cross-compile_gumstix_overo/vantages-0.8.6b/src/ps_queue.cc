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

#include "ps_queue.h"
#include "ps_task.h"
#include "ps_logger.h"
#include "ps_mutex_hdlr.h"
#include "ps_defs.h"

using namespace std;

PsQueue::PsQueue()
  : m_iMax(0),
    m_iCurrent(0)
{

}

PsQueue::~PsQueue()
{
  clear();
}

bool PsQueue::init(int p_iMax)
{
  clear();
  m_iMax = p_iMax;

  return true;
}

bool PsQueue::run()
{
  bool bRet = false;

  while (getRun())
  {
    bRet = true;

    PsTask *pTask = dequeue();
    if (NULL == pTask)
    {
      m_oMutex.wait(5000);
    }
    else
    {
      pTask->execute();
      delete pTask;
    }
  }

  return bRet;
}

PsQueue *PsQueue::dup()
{
  return new PsQueue();
}

bool PsQueue::clear()
{
  for (PsTaskIter_t tIter = m_oQueue.begin();
       m_oQueue.end() != tIter;
       tIter++)
  {
    delete (*tIter);
  }
  m_oQueue.clear();

  return true;
}

bool PsQueue::enqueue(PsTask &p_oTask)
{
  bool bRet = false;

  while (!bRet)
  //
  // CRITICAL SECTION BEGIN
  //
  {
    PsMutexHandler oMH(m_oMutex);

    if (m_iCurrent >= m_iMax)
    {
      ps_elog(PSL_WARNING, "Queue full, waiting...\n");
      m_oMutex.wait(5000);
    }
    else
    {
      m_oQueue.push_back(&p_oTask);
      m_iCurrent++;
      bRet = true;
    }

    m_oMutex.signal();
  }
  //
  // CRITICAL SECTION END
  //

  return bRet;
}

PsTask *PsQueue::dequeue()
{
  PsTask *pRet = NULL;

  if (m_iCurrent < 0)
  {
    ps_elog(PSL_CRITICAL, "Queue has less than 0 entries in it: Error!\n");
  }
  else if (0 == m_iCurrent)
  {
    ps_elog(PSL_DEBUG, "Queue is empty.\n");
  }
  else
  //
  // BEGIN CRITICAL SECTION
  //
  {
    PsMutexHandler oMH(m_oMutex);

    pRet = m_oQueue.front();
    m_oQueue.pop_front();
    m_iCurrent--;
    m_oMutex.signal();
  }
  //
  // CRITICAL SECTION END
  //

  return pRet;
}

PsMutex &PsQueue::getMutex()
{
  return m_oMutex;
}
