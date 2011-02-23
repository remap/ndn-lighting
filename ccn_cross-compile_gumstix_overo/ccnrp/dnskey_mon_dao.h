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

#ifndef _DNSKEY_MON_DAO_H
#define _DNSKEY_MON_DAO_H

#include "ps_dao.h"
#include "dnskey_defs.h"

class DnskeyMonDao : public PsDao
{
  // Member Variables
  private:
    bool m_bDelete;
    bool m_bUpdateNextPoll;
    int m_iScraperID;
    time_t m_tFirstPoll;
    time_t m_tLastPoll;
    time_t m_tNextPoll;
    dnskey_src_e m_eSubSrc;
    int m_iStart;
    int m_iCount;
    std::string m_sURL;
    std::string m_sName;

  public:
    static const char *s_kszDaoName;

    static const char *s_szSelectList;
    static const char *s_szSelect;
    static const char *s_szInsert;
    static const char *s_szUpdate;
    static const char *s_szUpdateNextPoll;
    static const char *s_szDelete;

  // Methods
  public:
    DnskeyMonDao();
    virtual ~DnskeyMonDao();

    bool getDeleteMode();
    void setDeleteMode(bool p_bDelete);

    int getScraperID();
    void setScraperID(int p_iScraperID);

    time_t getFirstPoll();
    void setFirstPoll(time_t p_tPoll);

    time_t getLastPoll();
    void setLastPoll(time_t p_tPoll);

    time_t getNextPoll();
    void setNextPoll(time_t p_tPoll);

    dnskey_src_e getSubSrc();
    void setSubSrc(dnskey_src_e p_eSubSrc);

    std::string &getURL();
    void setURL(std::string &p_sURL);

    std::string &getName();
    void setName(std::string &p_sName);

    int getStart();
    int getCount();
    void setRange(int p_iStart, int p_iCount);

    void updateNextPoll();

    virtual bool serialize();
    virtual bool deserialize();
    virtual bool deserialize(DaoList_t &p_oOutputList);

    virtual DnskeyMonDao *dup();
    virtual std::string daoName();

};

#endif
