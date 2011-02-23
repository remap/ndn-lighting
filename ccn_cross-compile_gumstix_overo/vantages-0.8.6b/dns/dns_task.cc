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

#include "dns_task.h"
#include "dns_packet.h"

DnsTask::DnsTask()
  : m_pQuery(NULL),
    m_pResponse(NULL),
    m_tTimeout(0),
    m_uIP(0),
    m_uPort(53)
{

}

DnsTask::~DnsTask()
{
  setQuery(NULL);
  setResponse(NULL);
}

DnsPacket *DnsTask::getQuery()
{
  return m_pQuery;
}

void DnsTask::setQuery(DnsPacket *p_pQuery)
{
  if (NULL != m_pQuery)
  {
    delete m_pQuery;
    m_pQuery = NULL;
  }

  m_pQuery = p_pQuery;
}

DnsPacket *DnsTask::getResponse()
{
  return m_pResponse;
}

void DnsTask::setResponse(DnsPacket *p_pResp)
{
  if (NULL != m_pResponse)
  {
    delete m_pResponse;
    m_pResponse = NULL;
  }

  m_pResponse = p_pResp;
}

time_t DnsTask::getTimeout()
{
  return m_tTimeout;
}

void DnsTask::setTimeout(time_t p_tTimeout)
{
  m_tTimeout = p_tTimeout;
}

uint32_t DnsTask::getNameserver()
{
  return m_uIP;
}

void DnsTask::setNameserver(uint32_t p_uIP)
{
  m_uIP = p_uIP;
}

uint16_t DnsTask::getPort()
{
  return m_uPort;
}

void DnsTask::setPort(uint16_t p_uPort)
{
  m_uPort = p_uPort;
}

