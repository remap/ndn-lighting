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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "ps_poller.h"
#include "dns_resolver.h"
#include "dns_packet.h"
#include "dns_dnskey.h"
#include "dns_name.h"
#include "ps_defs.h"
#include "dnskey_set_dao.h"
#include "dnskey_proc_dao.h"
#include "dao_factory.h"
#include "dnskey_poll_task.h"
#include "ps_url_parser.h"
#include "ps_config.h"
#include "ps_thrd.h"
#include "ps_poll_thrd.h"

using namespace std;

PsPoller::PsPoller()
  : m_bInit(false)
{
  /*
  PsConfig &oConf = PsConfig::getInstance();

  m_oRes.setConcurrency(40);
  const char *szConc = oConf.getValue(PS_CONFIG_RES_CONC);
  if (NULL != szConc)
  {
    m_oRes.setConcurrency((int) strtol(szConc, NULL, 10));
  }
  */
}

PsPoller::~PsPoller()
{
  m_oPool.kill();
}

bool PsPoller::init(int p_iResConc, int p_iThrdCount)
{
  bool bRet = false;

  m_oRes.setConcurrency(p_iResConc);
  m_oRes.setDO(true);
  m_oRes.setBuffSize(4096);

  PsPollThread oThr(&m_oPool);

  m_oPool.setSize(p_iThrdCount);
  if (!m_oPool.init(&oThr))
  {
    ps_elog(PSL_CRITICAL, "Unable to init pool.\n");
  }
  else
  {
    bRet = true;
  }

  return bRet;
}

bool PsPoller::poll(PsPollTaskIter_t p_tBegin,
                    PsPollTaskIter_t p_tEnd)
{
  bool bRet = false;

  m_oDnsTasks.clear();
  m_oWebTasks.clear();

  for (PsPollTaskIter_t tIter = p_tBegin;
       p_tEnd != tIter;
       tIter++)
  {
    PsPollTask *pTask = *tIter;
    string &sProto = pTask->getProto();
    if (sProto == "dnssec")
    {
      m_oDnsTasks.push_back(pTask);
    }
    else if (sProto == "http" || sProto == "https")
    {
      m_oWebTasks.push_back(pTask);
    }
    else
    {
      ps_elog(PSL_ERROR, "Task has unsupported protocol: '%s'\n", sProto.c_str());
    }
  }

  while (!m_oDnsTasks.empty()
         || !m_oWebTasks.empty()
         || m_oRes.hasTasks()
         || m_oPool.getNumFree() < m_oPool.getSize())
  {
    if (!pollDns() && !pollWeb())
    {
      sleep(1);
    }
  }

  bRet = true;

  return bRet;
}

bool PsPoller::pollDns()
{
  bool bRet = false;

  DnsTask *pTask = NULL;
  PsPollTask *pListTask = NULL;
//  if (!m_oDnsTasks.empty())
  {
    bool bSent = false;
    if (m_oRes.hasTasks())
    {
      bSent = true;

      for (int i = 0;
           i < m_oRes.getConcurrency()
           && m_oRes.hasTasks()
           && NULL != (pTask = m_oRes.recv());
           i++)
      {
        bRet = true;
        pListTask = dynamic_cast<PsPollTask *>(pTask);

        if (NULL == pListTask)
        {
          ps_elog(PSL_CRITICAL, "Unable to cast DnsTask to PsPollTask???\n");
        }
        else if (!pListTask->done())
        {
          if (!pListTask->execute())
          {
            ps_elog(PSL_INFO, "Unable to finish task... Dropping for zone: '%s'\n", pListTask->getURL().c_str());
          }
          else if (!m_oRes.send(pTask))
          {
            ps_elog(PSL_INFO, "Unable to re-send same task... Dropping for zone: '%s'\n", pListTask->getURL().c_str());
          }
        }
        else if (!pListTask->process())
        {
          ps_elog(PSL_INFO, "Unable to process... Dropping for zone: '%s'\n", pListTask->getURL().c_str());
        }

        pTask = NULL;
      }
    }

    for (int j = 0;
         j < m_oRes.getConcurrency()
         && !m_oDnsTasks.empty()
         && m_oRes.hasRoomToSend();
         j++)
    {
      bSent = true;

      pListTask = m_oDnsTasks.front();
      m_oDnsTasks.pop_front();
      if (pListTask->getProto() != "dnssec")
      {
        ps_elog(PSL_ERROR, "Protocol is not DNSSEC: '%s'\n", pListTask->getProto().c_str());
      }
      else
      {
        pTask = dynamic_cast<DnsTask *>(pListTask);
        if (NULL == pTask)
        {
          ps_elog(PSL_ERROR, "Unable to cast PsPollTask to DnsTask?\n");
        }
        else if (!pListTask->execute())
        {
          ps_elog(PSL_WARNING, "Unable to execute new task for zone '%s'... dropping.\n", pListTask->getURL().c_str());
        }
        else if (!m_oRes.send(pTask))
        {
          ps_elog(PSL_WARNING, "Unable to send task for zone: '%s'\n", pListTask->getURL().c_str());
        }
        pTask = NULL;
        bRet = true;
      }
    }
  }


  return bRet;
}

bool PsPoller::pollWeb()
{
  bool bRet = false;

  if (!m_oWebTasks.empty())
  {
    if (m_oPool.getNumFree() > 0)
    {
      PsThread *pThrd = m_oPool.checkOut();
      if (NULL == pThrd)
      {
        ps_elog(PSL_CRITICAL, "Got NULL thread from pool.\n");
      }
      else
      {
        PsPollTask *pTask = m_oWebTasks.front();
        m_oWebTasks.pop_front();
        pThrd->setTask(pTask);
        bRet = true;
      }
    }
  }

  return bRet;
}

