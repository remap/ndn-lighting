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

#ifndef _DNSKEY_DEFS_H
#define _DNSKEY_DEFS_H

#include <string>

#define DNSKEY_NAME_SPACE_ID 1

#define DNSKEY_CONFIG_ADMIN_PASS "dnskey_admin_pass"
#define DNSKEY_CONFIG_APP_PASS "dnskey_app_pass"
#define DNSKEY_CONFIG_MON_TIMEOUT "dnskey_mon_timeout"
#define DNSKEY_CONFIG_DATA_TIMEOUT "dnskey_data_timeout"
#define DNSKEY_CONFIG_CONSISTENCY_THRESHOLD "dnskey_consistency_threshold"

#define DNSKEY_DEFAULT_SECSPIDER_SCRAPER_ID 3
#define DNSKEY_DEFAULT_RIPE_SCRAPER_ID 4
#define DNSKEY_DEFAULT_MON_TIMEOUT 86400*30
#define DNSKEY_DEFAULT_DATA_TIMEOUT 86400*30

#define DNSKEY_HTTP_MIN_TIME_KEY "dnskey_min_time"
#define DNSKEY_HTTP_VERSION "dnskey_version"

#define DNSKEY_TA_FILE_POLICY_ID 1

#define DNSKEY_CURRENT_VERSION std::string("1.0")

typedef enum
{
  DNSKEY_SRC_PRE_INIT = 0,
  DNSKEY_SRC_USER,
  DNSKEY_SRC_PCAP
} dnskey_src_e;

#endif
