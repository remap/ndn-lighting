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

#ifndef _PS_POLL_TASK_H
#define _PS_POLL_TASK_H

#include <string>

#include "ps_task.h"
#include "ps_url_parser.h"

class PsCryptMgr;
class PsApp;

class PsPollTask : public PsTask
{
  // Member Variables
  private:
    std::string m_sURL;
    std::string m_sName;
    std::string m_sRawDaoName;
    std::string m_sProcDaoName;
    int m_iScraperID;
    int m_iScraperParamID;
    int m_iMaxRetries;
    int m_iRetry;
    PsUrlParser m_oParser;
    PsCryptMgr *m_pCryptMgr;
    PsApp &m_oOwnerApp;

  // Methods
  public:
    PsPollTask(PsApp &p_oOwnerApp);
    virtual ~PsPollTask();

    bool init(std::string &p_sURL,
              std::string &p_sName,
              const char *p_szRawDaoName,
              const char *p_szProcDaoName,
              int p_iScraperID,
              int p_iScraperParamID = -1);
    bool init(std::string &p_sURL,
              std::string &p_sName,
              std::string &p_sRawDaoName,
              std::string &p_sProcDaoName,
              int p_iScraperID,
              int p_iScraperParamID = -1);

    int getMaxRetries();
    void setMaxRetries(int p_iMax);

    int getRetry();
    void setRetry(int p_iRetry);

    std::string &getURL();
    void setURL(std::string &p_sURL);
    std::string &getProto();

    std::string &getName();
    void setName(std::string &p_sName);

    std::string &getRawDaoName();
    std::string &getProcDaoName();
    int getScraperID();
    int getScraperParamID();

    PsCryptMgr *getCryptMgr();
    void setCryptMgr(PsCryptMgr *p_pCryptMgr);

    PsApp &getOwnerApp();

    virtual bool done() = 0;
    virtual bool process() = 0;
};

#endif
