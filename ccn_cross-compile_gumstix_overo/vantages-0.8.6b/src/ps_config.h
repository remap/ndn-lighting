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

#ifndef _PS_CONFIG_H
#define _PS_CONFIG_H

#include <list>
#include <map>

#define PS_CONFIG_LINE_LEN           1024

class PsConfig
{
  // Enums and Typedefs
  typedef std::list<std::string> StringList_t;
  typedef StringList_t::iterator StringListIter_t;

  typedef std::map<std::string, StringList_t> ConfigMap_t;
  typedef ConfigMap_t::iterator               ConfigMapIter_t;

  // Member Variables
  private:
    char *m_szFile;
    const char *m_szError;
    ConfigMap_t m_oConfigMap;

    static PsConfig s_oInstance;

  // Methods
  private:
    PsConfig(PsConfig &p_oRHS);
    PsConfig &operator=(PsConfig &p_oRHS);

  public:
    PsConfig();
    virtual ~PsConfig();

    static PsConfig &getInstance();

    void clear();
    int load(char *p_szFile);
    int load(const char *p_szFile);

    const char *getValue(char *p_szKey);
    const char *getValue(const char *p_szKey);

    const char *getError();

  private:
    char *_trim(char *p_szLine, int p_iMaxLen);
    void _chomp(char *p_szLine, int p_iMaxLen);
};

#endif
