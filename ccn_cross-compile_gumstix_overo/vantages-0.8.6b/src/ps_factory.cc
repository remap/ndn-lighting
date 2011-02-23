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

#include "dao_factory.h"
#include "i_ps_dupable.h"

using namespace std;

PsFactory::PsFactory()
{

}

PsFactory::~PsFactory()
{
  reset();
}

bool PsFactory::reg(std::string p_sName, IPsDupable &p_oVal)
{
  bool bRet = false;

  IPsDupable *pVal = p_oVal.dup();
  PsFactoryMap_t::value_type tBlah(p_sName, pVal);
  bRet = m_oMap.insert(tBlah).second;
  if (!bRet)
  {
    delete pVal;
  }

  return bRet;
}

IPsDupable *PsFactory::create(std::string p_sName)
{
  IPsDupable *pRet = NULL;
  
  PsFactoryIter_t tIter = m_oMap.find(p_sName);
  if (m_oMap.end() != tIter)
  {
    pRet = tIter->second->dup();
  }

  return pRet;
}

void PsFactory::reset()
{
  for (PsFactoryIter_t tIter = m_oMap.begin();
       m_oMap.end() != tIter;
       tIter++)
  {
    delete tIter->second;
  }
  m_oMap.clear();
}
