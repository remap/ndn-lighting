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

#ifndef _PS_FACTORY_H
#define _PS_FACTORY_H

#include <stdio.h>

#include <map>
#include <string>
#include <stdio.h>

#include "i_ps_dupable.h"

template<typename K>
class PsFactory
{
  public:
    typedef typename std::map<K, IPsDupable *> PsFactoryMap_t;
    typedef typename PsFactoryMap_t::iterator PsFactoryIter_t;

  // Member Variables
  private:
    PsFactoryMap_t m_oMap;

  // Methods
  private:
    PsFactory &operator=(const PsFactory &p_oRHS);

  public:
    PsFactory();
    virtual ~PsFactory();

    bool reg(K p_sName, IPsDupable &p_oVal);
    virtual IPsDupable *create(K p_sName);

    void reset();
};


template<typename K>
PsFactory<K>::PsFactory()
{

}

template<typename K>
PsFactory<K>::~PsFactory()
{
  reset();
}

template<typename K>
bool PsFactory<K>::reg(K p_sName, IPsDupable &p_oVal)
{
/*
  bool bRet = false;

  IPsDupable *pVal = p_oVal.dup();
  PsFactoryMap_t::value_type tBlah(p_sName, pVal);
  bRet = m_oMap.insert(tBlah).second;
  if (!bRet)
  {
    delete pVal;
  }

  return bRet;
  m_oMap[p_sName] = p_oVal.dup();
*/
PsFactoryIter_t tIter = m_oMap.find(p_sName);
if (m_oMap.end() != tIter)
{
fprintf(stderr, "FOUND DUP.\n");
}
  m_oMap[p_sName] = &p_oVal;

  return true;
}

template<typename K>
IPsDupable *PsFactory<K>::create(K p_sName)
{
  IPsDupable *pRet = NULL;
  
  PsFactoryIter_t tIter = m_oMap.find(p_sName);
  if (m_oMap.end() != tIter)
  {
    pRet = tIter->second->dup();
  }

  return pRet;
}

template<typename K>
void PsFactory<K>::reset()
{
  for (PsFactoryIter_t tIter = m_oMap.begin();
       m_oMap.end() != tIter;
       tIter++)
  {
    delete tIter->second;
  }
  m_oMap.clear();
}

#endif
