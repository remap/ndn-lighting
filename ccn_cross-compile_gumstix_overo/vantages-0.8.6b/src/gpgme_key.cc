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

#include <string>

#include "gpgme_key.h"

using namespace std;

    gpgme_error_t m_tErr;
    std::list<gpgme_key_t> m_oKeyList;

GpgmeKey::GpgmeKey()
{

}

GpgmeKey::~GpgmeKey()
{
  for (list<gpgme_key_t>::iterator tIter = m_oKeyList.begin();
       m_oKeyList.end() != tIter;
       tIter++)
  {
    gpgme_key_release(*tIter);
  }
  m_oKeyList.clear();
}

bool GpgmeKey::init(gpgme_ctx_t &p_tCtx, std::string &p_sIdentifier, bool p_bSigner)
{
  bool bRet = false;

  gpgme_error_t tErr;
  tErr = gpgme_op_keylist_start(p_tCtx, p_sIdentifier.c_str(), p_bSigner);
  while (0 == tErr)
  {
    gpgme_key_t tKey;
    tErr = gpgme_op_keylist_next(p_tCtx, &tKey);
    if (0 == tErr)
    {
//      tErr = gpgme_signers_add(
    }
  }

  return bRet;
}


