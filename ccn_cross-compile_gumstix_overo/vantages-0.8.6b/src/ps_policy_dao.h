/*
 * Copyright (c) 2008,2009, University of California, Los Angeles and 
    ps_elog(PSL_CRITICAL, "m_sHomeDir = %s\n", m_sHomeDir.c_str());
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

#ifndef _PS_POLICY_DAO_H
#define _PS_POLICY_DAO_H

#include <string>

#include "ps_dao.h"

typedef enum
{
  PS_POLICY_UNKNOWN = 0,
  PS_POLICY_DENY,
  PS_POLICY_ALLOW
} ps_policy_e;

class PsPolicyDao : public PsDao
{
  // Types and enums
  typedef struct
  {
    std::string m_sRule;
    ps_policy_e m_ePolicy;
  } PolicyType_t;

  typedef std::list<PolicyType_t> PsPolicyList_t;
  typedef PsPolicyList_t::iterator PsPolicyIter_t;

  // Member Variables
  private:
    int m_iPolicyType;
    PsPolicyList_t m_oPolicyList;

  public:
    static const char *s_kszDaoName;
    static const char *s_szSelectPolicy;
    static const char *s_szDeletePolicy;
    static const char *s_szInsertPolicy;

  // Methods
  public:
    PsPolicyDao();
    virtual ~PsPolicyDao();

    int getType();
    bool setType(int p_iType);

    bool addRule(char *p_szRule, ps_policy_e p_ePolicy);
    bool addRule(const char *p_szRule, ps_policy_e p_ePolicy);
    bool addRule(std::string &p_sRule, ps_policy_e p_ePolicy);
    bool addDefaultRule(ps_policy_e p_ePolicy);

    PsPolicyIter_t begin();
    PsPolicyIter_t end();
    void clear();

    ps_policy_e check(char *p_szData);
    ps_policy_e check(const char *p_szData);
    ps_policy_e check(std::string &p_sData);

    void print();

    virtual bool serialize();
    virtual bool deserialize();
    virtual bool deserialize(DaoList_t &p_oOutputList);

    virtual std::string daoName();
    virtual PsPolicyDao *dup();
};

#endif
