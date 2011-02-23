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

#include "config.h"
#include <stdio.h>
#include <stdlib.h>

#include "dsync_ctx.h"
#include "ps_util.h"
#include "dao_factory.h"
#include "ps_url_parser.h"
#include "perl_scraper.h"
#include "http_query.h"
#include "ps_config.h"
#include "ps_logger.h"
#include "ps_defs.h"
#include "dnskey_defs.h"
#include "dsync_defs.h"
#include "dsync_app.h"
#include "dsync_ns_dao.h"
#include "dsync_zone_dao.h"
#include "dsync_rrset_dao.h"

using namespace std;

DsyncCtx::DsyncCtx()
{

}

DsyncCtx::~DsyncCtx()
{

}

bool DsyncCtx::process()
{
  bool bRet = true;

  clearOutput();
  string &sInput = getInput();
  if (sInput.empty())
  {
    appendOutput("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
		             "<html><body><center><h1>Dsync Administrative Page</center></h1>");
    appendOutput("<table border='0' width='100%'><tr align='center'>");
    appendOutput("<td><a href='?sub=info'>Zone Info</a></td>");
    appendOutput("<td><a href='?sub=add'>Add Zone</a></td>");
    appendOutput("<td><a href='?sub=delete'>Delete Zone</a></td>");
    appendOutput("</tr></table>");
    appendOutput("</body></html>");
  }
  else
  {
    string sTmp = "http://xyz/?";
    sTmp += sInput;
    PsUrlParser oParser;
    string sCmd;
    if (!oParser.parse(sTmp))
    {
      ps_elog(PSL_CRITICAL, "Unable to parse: '%s'\n", sTmp.c_str());
    }
    else if (oParser.getParam("sub", sCmd))
    {
      if (sCmd == "info")
      {
        bRet = infoPage(oParser);
      }
      else if (sCmd == "add")
      {
        bRet = addPage(oParser);
      }
      else if (sCmd == "delete")
      {
        bRet = delPage(oParser);
      }
      else
      {
        ps_elog(PSL_CRITICAL, "Unknown comamnd '%s'\n", sCmd.c_str());
        appendOutput("HTTP/1.1 500 OK\r\nContent-Type: text/html\r\n\r\n"
                     "<html><body>Unknown comamnd.\n</body></html>");
      }
    }
    else if (!oParser.getParam("cmd", sCmd))
    {
      ps_elog(PSL_CRITICAL, "Comamnd not specified.\n");
      appendOutput("HTTP/1.1 500 OK\r\nContent-Type: text/html\r\n\r\n"
                   "<html><body>Comamnd not specified.\n</body></html>");
    }
    else if (sCmd == "lookup")
    {
      bRet = lookup(oParser);
    }
    else if (sCmd == "addkey")
    {
      bRet = add(oParser);
    }
    else if (sCmd == "delkey")
    {
      bRet = del(oParser);
    }
    else
    {
      ps_elog(PSL_CRITICAL, "Unknown command: %s\n", sCmd.c_str());
      appendOutput("HTTP/1.1 500 OK\r\nContent-Type: text/html\r\n\r\n"
                   "<html><body>Unknown cmd.\n</body></html>");
    }
  }

  return bRet;
}

/*
const char *DsyncCtx::getHttpPath()
{
  return "/dsync";
}

const char *DsyncCtx::getHttpPass()
{
  return PsConfig::getInstance().getValue(DNSKEY_CONFIG_ADMIN_PASS);
}

DsyncCtx *DsyncCtx::createCtx()
{
  return new DsyncCtx(m_oApp);
}
*/

bool DsyncCtx::enabled()
{
  return (NULL != PsConfig::getInstance().getValue(DSYNC_CONFIG_APP_ENABLED));
}

bool DsyncCtx::init()
{
  return true;
}

std::string &DsyncCtx::crToBr(std::string &p_sStr)
{
  size_t u = 0;
  string sBR = "<br/>";
  while (string::npos != (u = p_sStr.find("\n", u)))
  {
    p_sStr.replace(u, 1, sBR);
  }

  return p_sStr;
}

bool DsyncCtx::infoPage(PsUrlParser& oParser)
{
  bool bRet = true;
  appendOutput("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
  appendOutput("<html><head><title>Lookup data</title></head><body>Lookup data:<p/> ");
  appendOutput("<form method=\"GET\">Enter name of zone: "
               "<input type=\"hidden\" name=\"cmd\" value=\"lookup\">"
	             "<input type=\"text\" name=\"name\"/><br/>"
	             "<input type=\"submit\" value=\"Lookup Name\"></form>");
  appendOutput("<hr/><i><a href='");
  appendOutput("/dsync");//getHttpPath());
  appendOutput("'>Main</a></i>");
  appendOutput("</body></html>");
  return bRet;
}

bool DsyncCtx::add(PsUrlParser& oParser)
{
  bool bRet = true;
  string sKeyStringKey = "keystring";
  string sKeyString;
  RRList_t keys;

  if (!oParser.getParam(sKeyStringKey, sKeyString))
  {
    ps_elog(PSL_CRITICAL, "Web Input Error: Key string not specified\n");
    appendOutput("HTTP/1.1 500 OK\r\nContent-type: text/html\r\n\r\n"
                "<html><body>Key string not specified.\n</body></html>");
  }

  sKeyString = HttpQuery::urlDecode(sKeyString);

  if (!parseKey(sKeyString, keys))
  {
    ps_elog(PSL_CRITICAL, "Unable to parse key string\n");
    appendOutput("HTTP/1.1 500 OK\r\nContent-type: text/html\r\n\r\n"
                "<html><body>Unable to parse key string.\n</body></html>");
  }
  else
  {
    DnsResolver oRes;
    oRes.setDO(true);
    oRes.setBuffSize(4096);
    DsyncInitTask* t = new DsyncInitTask((*(keys.begin()))->get_name()->toString());
    t->keyFromFile = keys;

    //very sloppy way to do this... but it should work
    //int tries = 0;
    while (!t->done())
    {
      t->execute();
      oRes.send(t);
      while (oRes.recv() == NULL)
      {
        sleep(1);
      }
      t->process();
    }
    
    DsyncZoneDao::clearList(keys);
    delete t;
  
    appendOutput("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
    appendOutput("<html><head><title>Added zone</title></head><body>Added zone.");
    appendOutput("<hr/><i><a href='");
    appendOutput("/dsync");
    appendOutput("'>Main</a></i>");
    appendOutput("</body></html>");
  }

  return bRet;
}

bool DsyncCtx::delPage(PsUrlParser& oParser)
{
  bool bRet = true;
  appendOutput("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
  appendOutput("<html><head><title>Delete zone</title></head><body>Delete zone:<p/> ");
  appendOutput("<b><i>WARNING: </i></b><font color=red> This form will delete <i>all</i> instances of a zone from the "
               "monitor database, including history and archived data</font>.");
  appendOutput("<form method=\"GET\">Enter name of zone: "
               "<input type=\"hidden\" name=\"cmd\" value=\"delkey\">"
	             "<input type=\"text\" name=\"name\"/><br/>"
	             "<input type=\"submit\" value=\"Delete Zone\"></form>");
  appendOutput("<hr/><i><a href='");
  appendOutput("/dsync");//getHttpPath());
  appendOutput("'>Main</a></i>");
  appendOutput("</body></html>");
  return bRet;
}

bool DsyncCtx::addPage(PsUrlParser& oParser)
{
  appendOutput("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
  appendOutput("<html><head><title>Submit new zone to monitor</title></head><body>Submit a new zone:<p/> ");
  appendOutput("<form method=\"POST\">Paste in a standard BIND format key, with no extra whitespace or comments: "
               "<input type=\"hidden\" name=\"cmd\" value=\"addkey\">"
               "<br/><textarea name=\"keystring\" rows=\"10\" cols=\"120\"></textarea><br/>"
	             "<input type=\"submit\" value=\"Add Zone\"></form>");
  appendOutput("<hr/><i><a href='");
  appendOutput("/dsync");
  appendOutput("'>Main</a></i>");
  appendOutput("</body></html>");
  return true;
}

bool DsyncCtx::deletePage(PsUrlParser& oParser)
{
  return true;
}

bool DsyncCtx::lookup(PsUrlParser& oParser)
{
  bool bRet = true;
  string sNameKey = "name";
  string sName;

  if (!oParser.getParam(sNameKey, sName))
  {
    ps_elog(PSL_ERROR, "Web Input Error: Zone name not specified\n");
    appendOutput("HTTP/1.1 500 OK\r\nContent-type: text/html\r\n\r\n"
                "<html><body>Zone name not specified.\n</body></html>");
  }

  else
  {

    if (sName.at(sName.length() - 1) != '.')
    {
      sName.append(".");
    }

    DsyncZoneDao zDao;
    zDao.zoneName = sName;
    if (!zDao.deserialize())
    {
      ps_elog(PSL_ERROR, "Unable to deserialize zone: %s\n", sName.c_str());
      appendOutput("HTTP/1.1 500 OK\r\nContent-type: text/html\r\n\r\n"
                "<html><body>Unable to deserialize zone.\n</body></html>");
    }
    else if ((zDao.parentName == "") && (zDao.getSyncState() == -1))
    {
      ps_elog(PSL_DEBUG, "Query for non-existant zone: %s\n", sName.c_str());
      appendOutput("HTTP/1.1 500 OK\r\nContent-type: text/html\r\n\r\n"
                "<html><body>Zone is not in database.\n</body></html>");
    }
    else
    {
      appendOutput("HTTP/1.1 500 OK\r\nContent-type: text/html\r\n\r\n"
                "<html><body>");
      zDao.printAll();
      
      std::vector<DsyncRrsetDao*>::iterator iter;
      std::vector<DsyncRrsetDao*> daos = zDao.getRrsets();

      appendOutput("Name: ");
      appendOutput(sName.c_str());
      appendOutput("<br/>State: ");
      if (zDao.getSyncState() == 0)
      {
        appendOutput("In-sync");
      }
      else if (zDao.getSyncState() == -1)
      {
        appendOutput("error state");
      }
      else if (zDao.getSyncState() == 1)
      {
        appendOutput("Key has changed, waiting on parent to update DS record\n");
      }
      else if (zDao.getSyncState() == 2)
      {
        appendOutput("Parent has updated DS. Please remove old dnskey\n");
      }
      else if (zDao.getSyncState() == 10)
      {
        appendOutput("Waiting DNSKEY TTL for caches to clear the old key\n");
      }
      else if (zDao.getSyncState() == 11)
      {
        appendOutput("Waiting DNSKEY TTL for caches to learn the new key\n");
      }
      else if (zDao.getSyncState() == 12)
      {
        appendOutput("Waiting DS TTL for caches to clear the old DS\n");
      }
      else
      {
        appendOutput("Unknown state\n");
      }
     
      /* 
      appendOutput("<br/>Nameservers:<ul>");
      for (NsVector::iterator nsi = zDao.getNsDao().getNsset().begin(); nsi != zDao.getNsDao().getNsset().end(); nsi++)
      {
        (*nsi).print();
        appendOutput("<li>");
        appendOutput((*nsi).name);
        appendOutput(": ");
        appendOutput((*nsi).ip);
        appendOutput("</li>");
      }
      appendOutput("</ul>");
      */
      appendOutput("<br/>Keys (current):<font size='2'><ul>");
      for (iter = daos.begin(); iter != daos.end(); iter++)
      {
        RRList_t keys = (*iter)->getKeyset();
        for (RRIter_t kit = keys.begin(); kit != keys.end(); kit++)
        {
          appendOutput("<li>");
          appendOutput(((DnsDnskey*)(*kit))->get_name()->toString().c_str());
          appendOutput("&nbsp; &nbsp; &nbsp;");
          appendOutput(((DnsDnskey*)(*kit))->ttl());
          appendOutput("&nbsp; &nbsp; &nbsp;");
          appendOutput(((DnsDnskey*)(*kit))->get_class());
          appendOutput("&nbsp; &nbsp; &nbsp;");
          appendOutput(((DnsDnskey*)(*kit))->type());
          appendOutput("&nbsp; &nbsp; &nbsp;");
          appendOutput(((DnsDnskey*)(*kit))->getFlags());
          appendOutput("&nbsp; &nbsp; &nbsp;");
          appendOutput(((DnsDnskey*)(*kit))->getProto());
          appendOutput("&nbsp; &nbsp; &nbsp;");
          appendOutput(((DnsDnskey*)(*kit))->getAlgo());
          appendOutput("&nbsp; &nbsp; &nbsp;");
          appendOutput(((DnsDnskey*)(*kit))->getKey().c_str());
          appendOutput("</li><br/>");
        }
      }
      appendOutput("</ul>");

      appendOutput("<font size='3'>DSs (current):<font size='2'><ul>");
      for (iter = daos.begin(); iter != daos.end(); iter++)
      {
        RRList_t dss = (*iter)->getDsset();
        for (RRIter_t dit = dss.begin(); dit != dss.end(); dit++)
        {
          appendOutput("<li>");
          appendOutput(((DnsDs*)(*dit))->get_name()->toString().c_str());
          appendOutput("&nbsp; &nbsp; &nbsp;");
          appendOutput(((DnsDs*)(*dit))->ttl());
          appendOutput("&nbsp; &nbsp; &nbsp;");
          appendOutput(((DnsDs*)(*dit))->get_class());
          appendOutput("&nbsp; &nbsp; &nbsp;");
          appendOutput(((DnsDs*)(*dit))->type());
          appendOutput("&nbsp; &nbsp; &nbsp;");
          appendOutput(((DnsDs*)(*dit))->getFlags());
          appendOutput("&nbsp; &nbsp; &nbsp;");
          appendOutput(((DnsDs*)(*dit))->getProto());
          appendOutput("&nbsp; &nbsp; &nbsp;");
          appendOutput(((DnsDs*)(*dit))->getAlgo());
          appendOutput("&nbsp; &nbsp; &nbsp;");
          appendOutput(((DnsDs*)(*dit))->getDig().c_str());
          appendOutput("</li>");
        }
      } 
      appendOutput("</ul><font size='3'>");
    }
    appendOutput("<hr/><i><a href='");
    appendOutput("/dsync");
    appendOutput("'>Main</a></i>");
    appendOutput("</body></html>");
  }
  return bRet;
}

bool DsyncCtx::del(PsUrlParser& oParser)
{
  bool bRet = true;
  string sNameKey = "name";
  string sName;

  if (!oParser.getParam(sNameKey, sName))
  {
    ps_elog(PSL_ERROR, "Web Input Error: Zone name not specified\n");
    appendOutput("HTTP/1.1 500 OK\r\nContent-type: text/html\r\n\r\n"
                "<html><body>Zone name not specified.\n</body></html>");
  }
  else
  {
    if (sName.at(sName.length() - 1) != '.')
    {
      sName.append(".");
    }
    DsyncZoneDao zDao;
    zDao.zoneName = sName;
    zDao.deserialize();
    zDao.deleteSelf();
    appendOutput("HTTP/1.1 500 OK\r\nContent-type: text/html\r\n\r\n"
                "<html><body>Deleted zone from database.\n</body></html>");
    appendOutput("<hr/><i><a href='");
    appendOutput("/dsync");
    appendOutput("'>Main</a></i>");
    appendOutput("</body></html>");
  }
  
  return bRet;
}

bool DsyncCtx::parseKey(string sKeyString, RRList_t &keys)
{
  bool bRet = true;
  string kName;
  string kClass;
  string kType;
  string kFlags;
  string kAlg;
  string kProto;
  string kKey;
  vector <u_char> binKey;
  
  DnsDnskey* dnskey = new DnsDnskey();
  
  try
  {
    stringstream ss (sKeyString);

    ss >> kName;
    ss >> kClass;
    ss >> kType;
    ss >> kFlags;
    ss >> kProto;
    ss >> kAlg;
    ss >> kKey;

    binKey = base64_decode(kKey);
    u_char bArr [binKey.size()];
    int i = 0;
    for (vector<u_char>::iterator it = binKey.begin();
        it != binKey.end();
        it++)
    {
      bArr[i++] = *it;
    }

    DnsName name(kName);
    dnskey->set_name(name);
    if (kClass != "IN")
    {
      ps_elog(PSL_ERROR, "Error adding new zone, class was not the expected DNS_CLASS_IN\n");
      bRet = false;
    }
    else
    {
      dnskey->set_class(DNS_CLASS_IN);
    }
    if (kType != "DNSKEY")
    {
      ps_elog(PSL_ERROR, "Error adding new zone, type must be DNS_RR_DNSKEY\n");
      bRet = false;
    }
    dnskey->setFlags((uint16_t)atoi(kFlags.c_str()));
    dnskey->setAlgo((uint8_t)atoi(kAlg.c_str()));
    dnskey->setProto((uint8_t)atoi(kProto.c_str()));
    dnskey->setKey(kKey);
    dnskey->setBinKey(bArr, binKey.size());

    printf("Parsed key as:\n");
    dnskey->print();

  }
  catch (...)
  {
    ps_elog(PSL_ERROR, "Error while adding new zone");
    bRet = false;
  }

  keys.push_back(dnskey);

  return bRet;
}
