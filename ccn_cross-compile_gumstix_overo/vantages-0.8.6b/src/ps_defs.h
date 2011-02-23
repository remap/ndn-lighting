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

#ifndef _PS_DEFS_H
#define _PS_DEFS_H

#include <list>
#include <string>
#include <map>

#define ps_log(X, ...) fprintf(stderr, "%s [%d] - " X, __FILE__, __LINE__, ##__VA_ARGS__)

#define PS_MAX_QUERY_LEN 1024*1024
#define PS_MAX_HTTP_QUERY_LEN PS_MAX_QUERY_LEN

#define PS_CONFIG_KEY_DB_CXN_STRING "db_cxn_string"
#define PS_CONFIG_HTTP_PORT "http_port"
#define PS_CONFIG_PID_FILE "pid_file"
#define PS_CONF_CURL_CA_FILE "ps_curl_ca_file"
#define PS_CONF_LOG_FILE "ps_log_file"
#define PS_CONF_LOG_LEVEL "ps_log_level"
#define PS_CONF_DEFAULT_USER "ps_default_user"

#define PS_CONFIG_TA_FILE_NAME "ta_file"
#define PS_CONFIG_TA_MAX_AGE "ta_max_key_age"
#define PS_CONFIG_RES_CONC "res_concurrency"
#define PS_NUM_POLLER_THREADS "num_poller_threads"
#define PS_CONFIG_DB_POLL_CHUNK_SIZE "db_poll_chunk_size"
#define PS_CONFIG_DNSKEY_APP_ENABLED "dnskey_app_enable"
#define DNSKEY_CONFIG_PCAP_ENABLE "dnskey_pcap_enable"
#define DNSKEY_CONFIG_PCAP_DEV "dnskey_pcap_dev"
#define DNSKEY_CONFIG_PCAP_PROGRAM "dnskey_pcap_program"


#define PS_PERL_SCRAPER_NAME "PERL_SCRAPER"
#define PS_DNSKEY_SCRAPER_NAME "DNS_DNSKEY_SCRAPER"

#define PS_DNSKEY_NAMESPACE "DNSKEYS_NS"

#define PS_DEFAULT_HTTP_PORT 9080
#define PS_HTTP_ZONE_LIST_KEY "dnskey_zone_list"

#define PS_F_RULE_THRESHOLD 5

#define PS_DEFAULT_DB_POLL_CHUNK_SIZE 10000

#define PS_HTTP_MAX_TIMEOUT_SECONDS 30

#define DNSKEY_PCAP_BUFF_SIZE 65535

#define DNSKEY_DEFAULT_DNS_SCRAPER_ID 2

class PsDao;
class RawDataDao;
class ProcessedDataDao;
class PsTask;
class PsPollTask;
class PsThread;
class PsApp;

enum ps_cache_type_e
{
  PS_RAW_DATA = 0,
  PS_RAW_REV_DATA,
  PS_PROC_DATA,
  PS_PROC_REV_DATA,
  PS_SCRAPER
};

enum ps_f_rule_result_e
{
  PS_F_RULE_PREINIT = 0,
  PS_F_RULE_CONFIRMED,
  PS_F_RULE_CONFLICT,
  PS_F_RULE_PROVISIONAL,
  PS_F_RULE_UNKNOWN
};

//typedef std::list<FriendCache *> FriendList_t;
//typedef FriendList_t::iterator FriendIter_t;

typedef std::list<PsDao *> DaoList_t;
typedef DaoList_t::iterator DaoIter_t;

typedef std::list<std::string> UrlList_t;
typedef UrlList_t::iterator UrlIter_t;

typedef std::list<int> IdList_t;
typedef IdList_t::iterator IdIter_t;

typedef std::list<RawDataDao *> RawDaoList_t;
typedef RawDaoList_t::iterator RawDaoIter_t;

typedef std::list<ProcessedDataDao *> ProcDaoList_t;
typedef ProcDaoList_t::iterator ProcDaoIter_t;

typedef std::list<std::string> StrList_t;
typedef StrList_t::iterator StrListIter_t;

typedef std::map<std::string, int> StrMap_t;
typedef StrMap_t::iterator StrIter_t;

typedef std::list<PsTask *> PsTaskList_t;
typedef PsTaskList_t::iterator PsTaskIter_t;

typedef std::list<PsPollTask *> PsPollTaskList_t;
typedef PsPollTaskList_t::iterator PsPollTaskIter_t;

typedef std::list<PsThread *> ThrdList_t;
typedef ThrdList_t::iterator ThrdIter_t;

typedef std::list<PsApp *> AppList_t;
typedef AppList_t::iterator AppIter_t;

#endif
