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
#include "ps_mutex_hdlr.h"
#include "ps_defs.h"

PsThreadPool::PsThreadPool()
  : m_bRun(false),
    m_iSize(10)
{

}

PsThreadPool::~PsThreadPool()
{
  kill();
}

bool PsThreadPool::init(PsThread *p_pThreadType /*= NULL*/)
{
  bool bRet = false;

  if (m_bRun)
  {
    ps_elog(PSL_CRITICAL, "Pool is already running.\n");
  }
  else
  {
/*
    if (NULL == p_pThreadType)
    {
      p_pThreadType = new PsThread(this);
    }
*/

    PsThread oDefaultThrd(this);
    m_bRun = true;
    bRet = true;
    for (int i = 0; i < getSize(); i++)
    {
//      PsThread *pThr = p_pThreadType->dup();
      PsThread *pThr = NULL;
      pThr = (NULL == p_pThreadType) ?  oDefaultThrd.dup() : p_pThreadType->dup();

      if (!pThr->start())
      {
        ps_elog(PSL_CRITICAL, "Unable to start thread, killing...\n");
        pThr->kill();
        delete pThr;
        bRet = false;
        m_bRun = false;
        break;
      }
      else
      {
        m_oThreads.push_back(pThr);
        m_oFree.push_back(pThr);
      }
    }
  }

/*
  if (NULL != p_pThreadType)
  {
    delete p_pThreadType;
    p_pThreadType = NULL;
  }
*/

  return bRet;
}

int PsThreadPool::getSize()
{
  return m_iSize;
}

void PsThreadPool::setSize(int p_iSize)
{
  m_iSize = p_iSize;
}

int PsThreadPool::getNumFree()
{
  return (int) m_oFree.size();
}

bool PsThreadPool::checkIn(PsThread &p_oThrd)
{
  bool bRet = false;

//  if (!p_oThrd.done())
//  {
//    ps_elog(PSL_CRITICAL, "Unable to check thread in until it is done.\n");
//  }
//  else
  //
  // BEGIN CRITICAL SECTION
  //
  {
    PsMutexHandler oHandler(m_oMutex);

    m_oFree.push_back(&p_oThrd);
    m_oMutex.signal();
    bRet = true;
  }
  //
  // END CRITICAL SECTION
  //

  return bRet;
}

PsThread *PsThreadPool::checkOut()
{
  PsThread *pRet = NULL;

  while (NULL == pRet && m_bRun)
  //
  // BEGIN CRITICAL SECTION
  //
  {
    PsMutexHandler oHandler(m_oMutex);

    if (!m_oFree.empty())
    {
      pRet = m_oFree.front();
      m_oFree.pop_front();
    }
    else
    {
      m_oMutex.wait(1000);
    }
  }
  //
  // END CRITICAL SECTION
  //

  return pRet;
}

bool PsThreadPool::kill()
{
  m_bRun = false;

  ThrdIter_t tIter;
  for (tIter = m_oThreads.begin();
       m_oThreads.end() != tIter;
       tIter++)
  {
    (*tIter)->kill();
  }

  for (tIter = m_oThreads.begin();
       m_oThreads.end() != tIter;
       tIter++)
  {
    (*tIter)->join();
    delete (*tIter);
  }

  m_oThreads.clear();
  m_oFree.clear();

  return !m_bRun;
}
