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

#ifndef _PS_LOGGER_H
#define _PS_LOGGER_H

#include <string>
#include <map>

#define ps_elog(XX_LVL, X, ...) PsLogger::log(__FILE__, __LINE__, XX_LVL, X, ##__VA_ARGS__)
#define ps_syslog(X, ...) PsLogger::sysLog(__FILE__, __LINE__, X, ##__VA_ARGS__)
enum PS_LOGLEVEL
{
  PSL_CRITICAL = 0,
  PSL_ERROR,
  PSL_WARNING,
  PSL_INFO,
  PSL_DEBUG
};

class PsLogger
{
  // Enums and types
  public:
    typedef std::map<std::string, int> LogLevelMap_t;
    typedef LogLevelMap_t::iterator LogLevelIter_t;

  // Member Variables
  private:
    static PsLogger s_oInstance;

    int m_iLevel;
    FILE *m_pFile;
    std::string *m_pLevelNames;
    std::string m_sFile;
    LogLevelMap_t m_oNameMap;

  // Methods
  public:
    PsLogger();
    virtual ~PsLogger();

    void close();

    int getLevel();
    void setLevel(int p_iLevel);

    const std::string &getFileName();
    bool setFileName(const char *p_szFile);

    FILE *getFile();
    std::string levelToStr(int p_iLevel);
    int strToLevel(std::string &p_sLevel);

    static PsLogger &getInstance();
    static void log(const char *p_szFile, int p_iLine, int p_iLevel, const char * p_szFmt, ...);
    static void sysLog(const char *p_szFile, int p_iLine, const char * p_szFmt, ...);
};

#endif
