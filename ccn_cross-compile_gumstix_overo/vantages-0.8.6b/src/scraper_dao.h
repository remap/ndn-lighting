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

#ifndef _SCAPER_DAO_H
#define _SCAPER_DAO_H

#include "ps_dao.h"
#include "ps_defs.h"

class ScraperDao : public PsDao
{
  // Member Variables
  public:
    static const char *s_kszDaoName;

  private:
    char *m_pScraperBin;
    size_t m_uBinLen;

    char *m_pData;
    size_t m_uLen;

    int m_iType;
    int m_iParamID;

    std::string m_sParam;
    std::string m_sResult;

    static const char *s_szSelectSQL;
    static const char *s_szSelectParamSQL;
    static const char *s_szInsertSQL;
    static const char *s_szInsertParamSQL;
    static const char *s_szUpdateSQL;

  // Methods
  public:
    ScraperDao();
    virtual ~ScraperDao();

    char *getScraperBin();
    size_t getBinLen();
    bool setScraperBin(char *p_pBin, size_t p_uLen);

    int getParamID();
    void setParamID(int p_iParamID);

    std::string &getParam();
    void setParam(std::string &p_sParam);

    char *getData();
    size_t getDataLen();
    bool setData(char *p_pData, size_t p_uLen);

    std::string &getResult();
    void setResult(std::string &p_sResult);

    int getType();
    void setType(int p_iType);

    virtual bool execute();

    virtual bool serialize();
    virtual bool deserialize();
    virtual bool deserialize(DaoList_t &p_oOutputList);

    virtual std::string daoName();
    virtual IPsDupable *dup();

  protected:
    bool serializeParam();
};

#endif
