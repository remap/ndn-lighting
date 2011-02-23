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

#ifndef _RAW_DATA_DAO_H
#define _RAW_DATA_DAO_H

#include <string>

#include "ps_dao.h"

class RawDataDao : public PsDao
{
  // Member Variables
  public:
    static const char *s_kszDaoName;
    static const char *s_kszInsertSQL;
    static const char *s_kszUpdateSQL;
    static const char *s_kszSelectSQL;

  private:
    std::string m_sName;
    std::string m_sSrc;
    time_t m_tDate;
//    char *m_pSig;
    char *m_pBuff;
//    size_t m_uSigLen;
    size_t m_uBuffLen;

  // Methods
  public:
    RawDataDao();
    virtual ~RawDataDao();

    std::string &getName();
    void setName(const std::string &p_sName);

    std::string &getSrc();
    void setSrc(const std::string &p_sName);

    time_t getDate();
    void setDate(time_t p_tDate);

/*
    char *getSig();
    size_t getSigLen();
    bool setSig(const char *p_pSig, size_t p_uSigLen);
*/

    char *getData();
    size_t getDataLen();
    bool setData(const char *p_pBuff, size_t p_uBuffLen);

    virtual std::string daoName();
    virtual bool serialize();
    virtual bool deserialize();
    virtual bool deserialize(DaoList_t &p_oOutputList);
    virtual RawDataDao *dup();
};

#endif
