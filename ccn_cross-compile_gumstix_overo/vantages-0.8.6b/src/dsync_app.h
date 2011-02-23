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

#ifndef _DSYNC_APP_H
#define _DSYNC_APP_H

#include "ps_app.h"
#include "dsync_defs.h"
#include "dns_resolver.h"
#include <iostream>
#include "ps_logger.h"
#include <fstream>
#include "base64.h"
#include "dsync_init_task.h"
#include "dsync_first_task.h"
#include "dsync_second_task.h"
#include "dsync_third_task.h"
#include "dsync_defs.h"
#include "dsync_zone_dao.h"
#include "dsync_dnskey_abstraction_task.h"
#include "dsync_ctx.h"

using namespace std;

class DsyncApp : public PsApp
{
  // Member Variables
  private:
  static const char *s_szHttpPath;
  DnsResolver oRes;
  float weighted_check;
  
  // Methods
  public:
    DsyncApp();
    virtual ~DsyncApp();
    virtual const char *getHttpPath();
    virtual const char *getHttpPass();
    virtual HttpListenerCtx *createCtx();
    virtual bool enabled();
    virtual bool init();
    virtual bool execute();
};

#endif

