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

#ifndef _DNS_TSIG_H
#define _DNS_TSIG_H

#include <stdint.h>

#include <string>
#include <vector>

#include "dns_rr.h"
#include "dns_defs.h"

class DnsName;
class DnsPacket;

class DnsTsig : public DnsRR
{
  // Typedefs and Enums
  private:
    typedef struct
    {
      uint16_t m_uTimePadding;
      uint32_t m_uTime;
      uint16_t m_uFudge;
      uint16_t m_uMacSize;
    } __attribute__((__packed__)) dns_tsig_header_1_t;

    typedef struct
    {
      uint16_t m_uOrigID;
      uint16_t m_uError;
      uint16_t m_uOtherLen;
    } __attribute__((__packed__)) dns_tsig_header_2_t;

    typedef struct
    {
      uint16_t m_uTimePadding;
      uint32_t m_uTime;
      uint16_t m_uFudge;
      uint16_t m_uError;
      uint16_t m_uOtherLen;
    } __attribute__((__packed__)) dns_tsig_vars_t;

  // Member Variables
  public:
    static const char *s_kszMd5HmacName;

  private:
    std::string m_sAlgoName;
    time_t m_tTime;
    uint16_t m_uFudge;
    uint16_t m_uMacSize;
    u_char *m_pMac;
    uint16_t m_uOrigID;
    uint16_t m_uError;
    uint16_t m_uOtherLen;
    u_char *m_pOtherData;
    u_char *m_pKey;
    size_t m_uKeyLen;

  // Methods
  public:
    DnsTsig();
    DnsTsig(DnsTsig &p_oRHS);
    virtual ~DnsTsig();

    virtual const DnsTsig &operator=(DnsTsig &p_oRHS);
    virtual bool operator==(DnsTsig &p_oRHS);

    // returns true if there are four bytes of RDATA
    virtual bool rdata_valid();

    std::string &getAlgoName();
    void setAlgoName(char *p_szAlgoName);
    void setAlgoName(const char *p_kszAlgoName);
    void setAlgoName(std::string &p_sAlgoName);

    time_t getTime();
    void setTime(time_t p_tTime);

    uint16_t getFudge();
    void setFudge(uint16_t p_uFudge);

    uint16_t getMacSize();
    u_char *getMac();
    void setMac(u_char *p_pMac, uint16_t p_uMacSize);

    uint16_t getOrigID();
    void setOrigID(uint16_t p_uOrigID);

    uint16_t getError();
    void setError(uint16_t p_uError);

    uint16_t getOtherLen();
    u_char *getOtherData();
    void setOtherData(u_char *p_pOther, uint16_t p_uOtherLen);

    size_t getKeyLen();
    u_char *getKey();
    void setKey(u_char *p_pKey, size_t p_uKeyLen);

    bool supportedAlgo(std::string &p_sAlgoName);

    virtual DnsTsig *dup();
    virtual void printRData();

    virtual bool parseRData(u_char *p_pMsg,
                            size_t p_uMsgLen,
                            u_char *p_pRData,
                            size_t p_uRDataLen);

    bool calc(DnsPacket &p_oPkt, bool p_bSetTime = true);
    bool verify(DnsPacket &p_oPkt, u_char *p_pPreviousMac, size_t p_uPreviousMacSize);

    bool loadKeyFromFile(char *p_szFile);
    bool loadKeyFromFile(const char *p_kszFile);
    bool loadKeyFromFile(std::string &p_sFile);

  protected:
    bool setRData();

    bool parsePrivateKey(std::vector<std::string> &p_oLines);
    bool parsePublicKey(std::vector<std::string> &p_oLines);
};

#endif
