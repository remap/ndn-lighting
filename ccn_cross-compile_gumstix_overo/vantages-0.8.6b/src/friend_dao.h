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

#ifndef _FRIEND_DAO_H
#define _FRIEND_DAO_H

#include <string>

#include "ps_dao.h"
#include "ps_defs.h"

class FriendDao : public PsDao
{
  // Member Variables
  public:
    static const char *s_kszDaoName;
    static const char *s_szSelectFriends;
    static const char *s_szInsertFriend;
    static const char *s_szDeleteFriend;

  private:
    int m_iFriendID;
    bool m_bDelete;
    time_t m_tFirstSeen;
    time_t m_tLastSeen;
    std::string m_sURL;
    std::string m_sName;
    std::string m_sPass;
    std::string m_sKey;

  // Methods
  public:
    FriendDao();
    virtual ~FriendDao();

    int getFriendID();
    void setFriendID(int p_iID);

    time_t getFirstSeen();
    void setFirstSeen(time_t p_tSeen);

    time_t getLastSeen();
    void setLastSeen(time_t p_tSeen);

    std::string &getURL();
    void setURL(std::string &p_sURL);

    std::string &getName();
    void setName(std::string &p_sName);

    std::string &getPass();
    void setPass(std::string &p_sPass);

    std::string &getKey();
    void setKey(std::string &p_sKey);

    bool getDelete();
    void setDelete(bool p_bDelete);

    virtual bool serialize();
    virtual bool deserialize();
    virtual bool deserialize(DaoList_t &p_oOutputList);

    virtual FriendDao *dup();
    virtual std::string daoName();
};

#endif
