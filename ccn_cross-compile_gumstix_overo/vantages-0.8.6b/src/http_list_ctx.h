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

#ifndef _HTTP_LIST_CTX_H
#define _HTTP_LIST_CTX_H

#include <string>
#include <list>

class HttpListenerCtx
{
  // Member Variables
  private:
    size_t m_uOutputOffset;
    bool m_bSending;
    bool m_bReady;
    bool m_bErr;
    std::string m_sInput;
    std::list<std::string> m_oOutList;

  // Methods
  public:
    HttpListenerCtx();
    virtual ~HttpListenerCtx();

    bool readyToSend();
    void setReadyToSend(bool p_bReady);

    bool sendingResponse();
    void sendingResponse(bool p_bSending);

    void clearInput();
    void appendInput(char *p_pBuff, int p_iLen);
    void appendInput(const char *p_pBuff, int p_iLen);
    std::string &getInput();

    virtual bool process();

    void clearOutput();
    void appendOutput(std::string &p_sOutput);
    void appendOutput(char *p_szOutput);
    void appendOutput(const char *p_szOutput);
    void appendOutput(int p_iOutput);
    std::list<std::string> &getOutput();

    bool done();

    bool error();
    void setError(bool p_bErr);
};

#endif
