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

#ifndef _PS_DAO_H
#define _PS_DAO_H

#include <string>
#include <list>

#include "i_ps_dupable.h"
#include "ps_mutex.h"
#include "ps_defs.h"

typedef struct sqlite3 sqlite3;
typedef struct sqlite3_stmt sqlite3_stmt;

class PsDao : public IPsDupable
{
  // Member Variables
  private:
    int m_iID;
    int m_iNsID;
    sqlite3 *m_pCxn;
    sqlite3_stmt *m_pStmt;
    char *m_szError;
    bool m_bConnected;
    std::string m_sQuery;
    std::string m_sSig;

    static int s_iMaxCxns;
    static std::string s_sConnectString;
    static std::list<sqlite3 *> s_oCxnList;
    static std::list<sqlite3 *> s_oFreeCxnList;
    static PsMutex s_oMutex;

  // Methods
  public:
    PsDao();
    virtual ~PsDao();

    const char *getError();

    int getID();
    void setID(int p_iID);

    int getNsID();
    void setNsID(int p_iNsID);

    std::string &getSig();
    bool setSig(std::string &p_sSig);

    virtual bool serialize() = 0;
    virtual bool deserialize() = 0;
    virtual bool deserialize(DaoList_t &p_oOutputList) = 0;

    virtual std::string daoName() = 0;

    static std::string getConnectString();
    static void setConnectString(std::string &p_sConnectString);
    static void clearList(DaoList_t &p_oList);

static bool finalize();

  protected:

    bool connect(const char *p_szFile = __FILE__, int p_iLine = __LINE__);
    bool disconnect();
    bool clearQuery();
    bool isConnected();

    bool prepareQuery(const char *p_szQuery);
    bool prepareQuery(std::string &p_sQuery);

    bool setInt(int p_iIndex, int p_iVal);
    bool setDouble(int p_iIndex, double p_dVal);
    bool setStr(int p_iIndex, std::string &p_sVal);
    bool setBlob(int p_iIndex, char *p_pBuff, size_t p_uLen);
    bool setNULL(int p_iIndex);

    bool exec();

    bool update();
    bool next();

    int getInt(int p_iCol);
    double getDouble(int p_iCol);
    bool getStr(int p_iCol, std::string &p_sOutput);
    bool getBlob(int p_iCol, char **p_ppOutput, int &p_iOutLen);
    bool isNULL(int p_iCol);

    int getLastID();
};

#endif
