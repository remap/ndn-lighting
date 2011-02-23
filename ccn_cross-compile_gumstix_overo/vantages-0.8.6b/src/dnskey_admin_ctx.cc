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

#include "dnskey_admin_ctx.h"
#include "ps_util.h"
#include "dnskey_proc_dao.h"
#include "raw_data_dao.h"
#include "dao_factory.h"
#include "ps_url_parser.h"
#include "dnskey_app.h"
#include "perl_scraper.h"
#include "http_query.h"
#include "dnskey_consist_dao.h"
#include "ps_config.h"
#include "friend_dao.h"
#include "dnskey_mon_dao.h"
#include "ps_policy_dao.h"
#include "gpgme_crypt_mgr.h"
#include "ps_logger.h"
#include "ps_defs.h"
#include "dnskey_defs.h"

using namespace std;

DnskeyAdminCtx::DnskeyAdminCtx(DnskeyApp &p_oApp)
  : m_oApp(p_oApp)
{

}

DnskeyAdminCtx::~DnskeyAdminCtx()
{

}

bool DnskeyAdminCtx::process()
{
  bool bRet = true;

  clearOutput();
  string &sInput = getInput();
  if (sInput.empty())
  {
    appendOutput("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
		             "<html><body><h1>Vantages Administrative Page</h1>");
    appendOutput("<table border='0' width='100%'><tr align='center'>");
    appendOutput("<td><a href='?sub=data'>Lookup Data</a></td>");
    appendOutput("<td><a href='?sub=poll'>Trigger Poll</a></td>");
    appendOutput("<td><a href='?sub=sub'>Submit New Data</a></td>");
    appendOutput("<td><a href='?sub=friend'>Friends</a></td>");
    appendOutput("<td><a href='?sub=monlist'>Monitored URLs</a></td>");
    appendOutput("<td><a href='?sub=ta-file'>Trust-Anchor Config</a></td>");
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
      if (sCmd == "data")
      {
        bRet = dataPage(oParser);
      }
      else if (sCmd == "poll")
      {
        bRet = pollPage(oParser);
      }
      else if (sCmd == "sub")
      {
        bRet = submissionPage(oParser);
      }
      else if (sCmd == "friend")
      {
        bRet = friendPage(oParser);
      }
      else if (sCmd == "monlist")
      {
        bRet = monPage(oParser);
      }
      else if (sCmd == "ta-file")
      {
        bRet = taFilePage(oParser);
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
    else if (sCmd == "add")
    {
      bRet = add(oParser);
    }
    else if (sCmd == "poll")
    {
      bRet = poll(oParser);
    }
    else if (sCmd == "add-friend")
    {
      bRet = addFriend(oParser);
    }
    else if (sCmd == "rem-friend")
    {
      bRet = remFriend(oParser);
    }
    else if (sCmd == "ta-file")
    {
      bRet = setTaPolicy(oParser);
    }
    else
    {
      ps_elog(PSL_CRITICAL, "Unknown command: %s\n", sCmd.c_str());
      appendOutput("HTTP/1.1 500 OK\r\nContent-Type: text/html\r\n\r\n"
                   "<html><body>Unknonw cmd.\n</body></html>");
    }
  }

  return bRet;
}

const char *DnskeyAdminCtx::getHttpPath()
{
  return "/dnskey-admin";
}

const char *DnskeyAdminCtx::getHttpPass()
{
  return PsConfig::getInstance().getValue(DNSKEY_CONFIG_ADMIN_PASS);
}

DnskeyAdminCtx *DnskeyAdminCtx::createCtx()
{
  return new DnskeyAdminCtx(m_oApp);
}

bool DnskeyAdminCtx::enabled()
{
  return (NULL != PsConfig::getInstance().getValue(PS_CONFIG_DNSKEY_APP_ENABLED));
}

bool DnskeyAdminCtx::init()
{
  return true;
}

bool DnskeyAdminCtx::lookup(PsUrlParser &p_oParser)
{
  bool bRet = true;

//ps_elog(PSL_CRITICAL, "PARSED: '%s'\n", sTmp.c_str());
  string sTypeKey = "type";
  string sNameKey = "name";
  string sType;
  string sName;
  if (!p_oParser.getParam(sTypeKey, sType))
  {
    ps_elog(PSL_WARNING, "Type not specified.\n");
    appendOutput("HTTP/1.1 500 OK\r\nContent-Type: text/html\r\n\r\n"
                 "<html><body>Type not specified.\n</body></html>");
  }
  else if (!p_oParser.getParam(sNameKey, sName))
  {
    ps_elog(PSL_CRITICAL, "Data name not specified.\n");
    appendOutput("HTTP/1.1 500 OK\r\nContent-Type: text/html\r\n\r\n"
                 "<html><body>Data name not specified.\n</body></html>");
  }
  else
  {
    try
    {
      PsDao *pDao = NULL;
      if (sType == "raw")
      {
        RawDataDao *pRaw = dynamic_cast<RawDataDao *>(DaoFactory::getInstance().create(RawDataDao::s_kszDaoName));
        if (NULL == pRaw)
        {
          ps_elog(PSL_CRITICAL, "Unable to get Raw Dao.\n");
          appendOutput("HTTP/1.1 500 OK\r\nContent-Type: text/html\r\n\r\n"
                       "<html><body>Unable to get Raw Dao.\n</body></html>");
        }
        else
        {
          pRaw->setName(sName);
          pRaw->setNsID(1);
          pDao = pRaw;
        }
      }
      else if (sType == "dnskey")
      {
        DnskeyProcDao *pProc = (DnskeyProcDao *)(DaoFactory::getInstance().create(DnskeyProcDao::s_kszDaoName));
        if (NULL == pProc)
        {
          ps_elog(PSL_CRITICAL, "Unable to get dnskey DAO.\n");
          appendOutput("HTTP/1.1 500 OK\r\nContent-Type: text/html\r\n\r\n"
                       "<html><body>Unable to get dnskey DAO.\n</body></html>");
        }
        else
        {
          pProc->setScraper(NULL);
          pProc->setScraperID(-1);
          pProc->setName(sName);
          pDao = pProc;
        }
      }
      else if (sType == "consistency")
      {
        DnskeyConsistencyDao *pCons = (DnskeyConsistencyDao *)(DaoFactory::getInstance().create(DnskeyConsistencyDao::s_kszDaoName));
        if (NULL == pCons)
        {
          ps_elog(PSL_CRITICAL, "Unable to get consistency DAO.\n");
          appendOutput("HTTP/1.1 500 OK\r\nContent-Type: text/html\r\n\r\n"
                       "<html><body>Unable to get dnskey DAO.\n</body></html>");
        }
        else
        {
          pCons->setName(sName);
          pDao = pCons;
        }
      }
      else
      {
        ps_elog(PSL_CRITICAL, "Data type not recognized: %s.\n", sType.c_str());
        appendOutput("HTTP/1.1 500 OK\r\nContent-Type: text/html\r\n\r\n"
                     "<html><body>Data type ");
        appendOutput(sType);
        appendOutput("not recognized.\n</body></html>");
      }

      if (NULL != pDao)
      {
        DaoList_t oList;
        if (!pDao->deserialize(oList))
        {
          ps_elog(PSL_CRITICAL, "Unable to get deserialize DAO.\n");
          appendOutput("HTTP/1.1 500 OK\r\nContent-Type: text/html\r\n\r\n"
                       "<html><body>Unable deserialize DAO.\n</body></html>");
        }
        else
        {
          appendOutput("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
          appendOutput("<html><body>Data for ");
          appendOutput(sType);
          appendOutput(" Dao: &quot;");
          appendOutput(sName);
          appendOutput("&quot;");
          appendOutput("<p/><table width=\"100%\" border=\"1\">");
          if (sType == "raw")
          {
            appendOutput("<td>Name</td><td>URL</td><td>Data</td><td>Signature</td>");
          }
          else if (sType == "dnskey")
          {
            appendOutput("<td>Name</td><td>Raw IDs</td><td>Proc IDs</td><td>Scraper ID</td><td>Data</td><td>Signature</td>");
          }
          else if (sType == "consistency")
          {
            DnskeyConsistencyDao *pCDao = NULL;
            for (DaoIter_t tIter1 = oList.begin();
                 oList.end() != tIter1;
                 tIter1++)
            {
              DnskeyConsistencyDao *pTmpDao = (DnskeyConsistencyDao *) *tIter1;
              if (NULL == pCDao || pTmpDao->getDate() > pCDao->getDate())
              {
                pCDao = pTmpDao;
              }
            }

            if (NULL != pCDao)
            {
              int iID = pCDao->getID();
              pCDao = (DnskeyConsistencyDao *) DaoFactory::getInstance().create(pCDao->daoName());
              PsDao::clearList(oList);
              pCDao->setID(iID);
              pCDao->deserialize();
              oList.push_back(pCDao);
            }

            appendOutput("<td>Name</td><td>Date</td><td>Consistency</td><td>Data</td>");
          }

          if (oList.empty())
          {
            appendOutput("<tr><td colspan='4'><font color='red'>No Match for: ");
            appendOutput(sName.c_str());
            appendOutput("</td></tr>");
          }
          else
          {
            char sz[11];
            for (DaoIter_t tIter = oList.begin();
                 oList.end() != tIter;
                 tIter++)
            {
              if (sType == "raw")
              {
                char *szData = ((RawDataDao *) *tIter)->getData();
                string sData = (NULL == szData) ? "" : szData;
                addSpaces(sData);
                string sSig = ((RawDataDao *) *tIter)->getSig();

                appendOutput("<tr>");
                appendOutput("<td valign='top'>");
                appendOutput(((RawDataDao *) *tIter)->getName());
                appendOutput("</td>");
                appendOutput("<td valign='top'>");
                appendOutput(((RawDataDao *) *tIter)->getSrc());
                appendOutput("</td>");
                appendOutput("<td valign='top'><tt>");
                appendOutput(sData);
                appendOutput("</tt></td>");
                appendOutput("<td valign='top'><tt>");
                appendOutput(crToBr(sSig));
                appendOutput("</tt></td>");
                appendOutput("</tr>");
              }
              else if (sType == "dnskey")
              {
                DnskeyProcDao *pPDao = (DnskeyProcDao *)*tIter;
                string sData = pPDao->getData();
                string sSig = pPDao->getSig();
                addSpaces(sData);

                appendOutput("<tr>");
                appendOutput("<td valign='top'>");
                appendOutput(pPDao->getName());
                appendOutput("</td>");
                appendOutput("<td valign='top'>");
                for (IdIter_t tRawIter = pPDao->beginRawIDs();
                     pPDao->endRawIDs() != tRawIter;
                     tRawIter++)
                {
                  memset(sz, 0, 11);
                  sprintf(sz, "%d", *tRawIter);
                  appendOutput(sz);
                  appendOutput(", ");
                }
                appendOutput("</td>");
                appendOutput("<td valign='top'>");
                for (IdIter_t tProcIter = pPDao->beginProcIDs();
                     pPDao->endProcIDs() != tProcIter;
                     tProcIter++)
                {
                  memset(sz, 0, 11);
                  sprintf(sz, "%d", *tProcIter);
                  appendOutput(sz);
                  appendOutput(", ");
                }
                appendOutput("</td>");
                appendOutput("<td valign='top'>");
                memset(sz, 0, 11);
                sprintf(sz, "%d", pPDao->getScraperID());
                appendOutput(sz);
                appendOutput("</td>");
                appendOutput("<td valign='top'><tt>");
                appendOutput(sData);
                appendOutput("</tt></td>");
                appendOutput("<td valign='top'><tt>");
                appendOutput(crToBr(sSig));
                appendOutput("</tt></td>");
                appendOutput("</tr>");
              }
              else if (sType == "consistency")
              {
                DnskeyConsistencyDao *pConDao = (DnskeyConsistencyDao *) *tIter;
                string sData = pConDao->getData();
                string sSig = pConDao->getSig();
                size_t uPos = sData.find("|");
                if (string::npos == uPos)
                {
                  appendOutput("<tr><td colspan='3'>Unable to find data for dao: ");
                  appendOutput(pConDao->getName());
                  appendOutput("</td></tr>");
                }
                else
                {
                  string sCode = sData.substr(0, uPos);
                  string sNewData = sData.substr(uPos + 1, string::npos);
                  char szDate[11];
                  memset(szDate, 0, 11);
                  sprintf(szDate, "%d", (int) pConDao->getDate());
                  addSpaces(sNewData);

                  appendOutput("<tr>");
                  appendOutput("<td valign='top'>");
                  appendOutput(pConDao->getName());
                  appendOutput("</td>");
                  appendOutput("<td valign='top'>");
                  appendOutput(szDate);
                  appendOutput("</td>");

                  appendOutput("<td valign='top'>");
                  appendOutput(sCode);
                  appendOutput("</td>");
                  appendOutput("<td valign='top'><tt>");
                  appendOutput(sNewData);
                  appendOutput("</tt></td>");
                  appendOutput("<td valign='top'><tt>");
                  appendOutput(crToBr(sSig));
                  appendOutput("</tt></td>");
                  appendOutput("</tr>");
                }
              }
            }
          }
          appendOutput("</table>");
          appendOutput("<hr/><i><a href='./");
          appendOutput(getHttpPath());
          appendOutput("'>Main</a></i>");
          appendOutput("</body></html>");
        }
        PsDao::clearList(oList);

        delete pDao;
      }
    }
    catch (...)
    {
      ps_elog(PSL_CRITICAL, "Caught Exception.\n");
      clearOutput();
      appendOutput("HTTP/1.1 500 OK\r\nContent-Type: text/html\r\n\r\n"
                   "<html><body>Internal error\n</body></html>");
    }
  }

  return bRet;
}

bool DnskeyAdminCtx::add(PsUrlParser &p_oParser)
{
  bool bRet = true;

  bool bTrouble = false;
  string sNameKey = "name";
  string sUrlKey = "url";
  string sName;
  string sUrl;
  // SHamelessly hardcoded.
  int iScraperID = 2;
  PsUrlParser oTargetParser;

  if (!p_oParser.getParam(sNameKey, sName))
  {
    ps_elog(PSL_CRITICAL, "Data name not specified.\n");
    appendOutput("HTTP/1.1 500 OK\r\nContent-Type: text/html\r\n\r\n"
                 "<html><body>Data name not specified.\n</body></html>");
  }
  else if (!p_oParser.getParam(sUrlKey, sUrl))
  {
    ps_elog(PSL_CRITICAL, "Data URL not specified.\n");
    appendOutput("HTTP/1.1 500 OK\r\nContent-Type: text/html\r\n\r\n"
                 "<html><body>Data URL not specified.\n</body></html>");
  }
  else if ((sUrl = HttpQuery::urlDecode(sUrl)) == "" || !oTargetParser.parse(sUrl))
  {
    ps_elog(PSL_CRITICAL, "Unable to parse input URL '%s'.\n", sUrl.c_str());
    appendOutput("HTTP/1.1 500 OK\r\nContent-Type: text/html\r\n\r\n"
                 "<html><body>Malformed URL.\n</body></html>");
  }
  else if (oTargetParser.getProto() != "dns"
           && oTargetParser.getProto() != "dnssec")
  {
    string sPerl;
    string sPerlKey = "scraper";
    string sID;
    string sIdKey = "scraperid";
    bool bScraper = p_oParser.getParam(sPerlKey, sPerl);
    bool bScraperID = p_oParser.getParam(sIdKey, sID);
    if (!bScraper && !bScraperID)
    {
      ps_elog(PSL_CRITICAL, "Must specify a parser for HTTP.\n");
      appendOutput("HTTP/1.1 500 OK\r\nContent-Type: text/html\r\n\r\n"
                   "<html><body>Must specify a parser for HTTP.\n</body></html>");
      bTrouble = true;
    }
    else if (bScraper && bScraperID && sPerl != "" && sID != "")
    {
      ps_elog(PSL_CRITICAL, "Cannot specify a parser AND a parser ID for HTTP.\n");
      appendOutput("HTTP/1.1 500 OK\r\nContent-Type: text/html\r\n\r\n"
                   "<html><body>Cannot specify a parser AND a parser ID for HTTP.\n</body></html>");
      bTrouble = true;
    }
    else
    {
      PerlScraper *pScraper = dynamic_cast<PerlScraper *>(DaoFactory::getInstance().create(PerlScraper::s_kszDaoName));
      if (NULL == pScraper)
      {
        ps_elog(PSL_CRITICAL, "Unable to get perl scraper.\n");
        appendOutput("HTTP/1.1 500 OK\r\nContent-Type: text/html\r\n\r\n"
                     "<html><body>Internal error\n</body></html>");
        bTrouble = true;
      }
      else
      {
        iScraperID = -1;
        if (sID != "")
        {
          iScraperID = (int) strtol(sID.c_str(), NULL, 10);
        }
        string sParamKey = "scraperparam";
        string sParam;
        p_oParser.getParam(sParamKey, sParam);

        pScraper->setType(1);
        sPerl = HttpQuery::urlDecode(sPerl);
        if (iScraperID > -1)
        {
          pScraper->setID(iScraperID);
        }
        else
        {
          pScraper->setScraperBin((char *) sPerl.c_str(), sPerl.size());
        }
        pScraper->setParam(sParam);
        pScraper->deserialize();
        if (!pScraper->serialize())
        {
          ps_elog(PSL_CRITICAL, "Unable to serialize perl scraper.\n");
          appendOutput("HTTP/1.1 500 OK\r\nContent-Type: text/html\r\n\r\n"
                       "<html><body>Internal error\n</body></html>");
          bTrouble = true;
        }
        else
        {
          iScraperID = pScraper->getID();
        }

        delete pScraper;
        pScraper = NULL;
      }
    }
  }

  if (!bTrouble)
  {
    sName = HttpQuery::urlDecode(sName);
    sUrl = HttpQuery::urlDecode(sUrl);
    if (m_oApp.addNewTarget(sName, sUrl, iScraperID, DNSKEY_SRC_USER))
    {
      char sz[11];
      memset(sz, 0, 11);
      sprintf(sz, "%d", iScraperID);
      appendOutput("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
      appendOutput("<html><body>Added new monitor target<p/> ");
      appendOutput("<table border='1'><tr><td><b>Name</b></td><td><b>URL</b></td><td><b>Scraper ID</b></td></tr>\n");
      appendOutput("<tr><td>");
      appendOutput(sName);
      appendOutput("</td><td>");
      appendOutput(sUrl);
      appendOutput("</td><td>");
      appendOutput(sz);
      appendOutput("</td></tr>\n");
      appendOutput("<hr/><i><a href='./");
      appendOutput(getHttpPath());
      appendOutput("'>Main</a></i>");
      appendOutput("</body></html>\n");
    }
    else
    {
      ps_elog(PSL_CRITICAL, "Unable to add to application for monitoring.\n");
      appendOutput("HTTP/1.1 500 OK\r\nContent-Type: text/html\r\n\r\n"
                       "<html><body>Internal error\n</body></html>");
    }
  }

  return bRet;
}

bool DnskeyAdminCtx::poll(PsUrlParser &p_oParser)
{
  bool bRet = true;

  string sNameKey = "name";
  string sName;
  string sUrlKey = "url";
  string sUrl;

  p_oParser.getParam(sNameKey, sName);
  p_oParser.getParam(sUrlKey, sUrl);

  DnskeyMonDao *pDao = static_cast<DnskeyMonDao *>(DaoFactory::getInstance().create(DnskeyMonDao::s_kszDaoName));
  if (NULL == pDao)
  {
    ps_elog(PSL_CRITICAL, "Unable to get DnskeyMonDao.\n");
    appendOutput("HTTP/1.1 500 OK\r\nContent-Type: text/html\r\n\r\n"
                 "<html><body>Internal server error\n</body></html>");
  }
  else
  {
    sName = HttpQuery::urlDecode(sName);
    sUrl = HttpQuery::urlDecode(sUrl);

    pDao->setNextPoll(0);
    pDao->updateNextPoll();
    pDao->setName(sName);
    pDao->setURL(sUrl);

    if (!pDao->serialize())
    {
      ps_elog(PSL_ERROR, "Unable to serialize DAO to poll now.\n");
      appendOutput("HTTP/1.1 500 OK\r\nContent-Type: text/html\r\n\r\n"
                   "<html><body>Internal server error\n</body></html>");
    }
    else
    {
      ps_elog(PSL_INFO, "Rescheduling polling for: '%s', '%s'\n", sName.c_str(), sUrl.c_str());
      appendOutput("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
		               "<html><head><title>On-Demand Poll</title></head><body>");
      if (sName == "" && sUrl == "")
      {
        appendOutput("<font size='+1'>Scheduling <i>all</i> URLs for immediate polling.</font>");
      }
      else
      {
        appendOutput("<font size='+1'>Scheduling Name / URL: &quot;");
        appendOutput(sName);
        appendOutput("&quot; / &quot;");
        appendOutput(sUrl);
        appendOutput("&quot; for immediate polling.</font>");
        appendOutput("<hr/><i><a href='./");
        appendOutput(getHttpPath());
        appendOutput("'>Main</a></i>");
        appendOutput("</body></html>");
      }
    }
  }

  if (NULL != pDao)
  {
    delete pDao;
    pDao = NULL;
  }


  return bRet;
}

bool DnskeyAdminCtx::execute()
{
  while (getRun())
  {
    sleep(10);
  }

  return true;
}

bool DnskeyAdminCtx::addSpaces(std::string &p_sStr, size_t p_uRunLen /*= 50*/)
{
  bool bRet = false;

  for (size_t u = p_uRunLen - 1; u < p_sStr.size(); u += p_uRunLen)
  {
    bRet = true;
    p_sStr.insert(u, " ");
  }
   return bRet;
}

std::string &DnskeyAdminCtx::crToBr(std::string &p_sStr)
{
  size_t u = 0;
  string sBR = "<br/>";
  while (string::npos != (u = p_sStr.find("\n", u)))
  {
    p_sStr.replace(u, 1, sBR);
  }

  return p_sStr;
}

bool DnskeyAdminCtx::dataPage(PsUrlParser &p_oParser)
{
  appendOutput("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
  appendOutput("<html><head><title>Lookup data</title></head><body>Lookup data:<p/> ");
  appendOutput("<form method=\"GET\">Enter name and type of data: "
               "<input type=\"hidden\" name=\"cmd\" value=\"lookup\">"
	             "<input type=\"text\" name=\"name\"/><br/>"
               "<input type=\"radio\" name=\"type\" value=\"raw\">Raw<br/>"
               "<input type=\"radio\" name=\"type\" value=\"dnskey\">DNSKEY<br/>"
               "<input type=\"radio\" name=\"type\" value=\"consistency\">Consistency<br/>"
	             "<input type=\"submit\" value=\"Lookup Name\"></form>");
  appendOutput("<hr/><i><a href='");
  appendOutput(getHttpPath());
  appendOutput("'>Main</a></i>");
  appendOutput("</body></html>");
  return true;
}

bool DnskeyAdminCtx::submissionPage(PsUrlParser &p_oParser)
{
  appendOutput("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
  appendOutput("<html><head><title>Submit new data source</title></head><body>Submit new data source:<p/> ");
  appendOutput("<form method=\"POST\">Enter information for new zone to poll: "
               "<input type=\"hidden\" name=\"cmd\" value=\"add\">"
               "Name of zone: <input type=\"text\" name=\"name\"/><br/>"
               "URL of zone: <input type=\"text\" name=\"url\"/><p/>"
               "<i>Note: The fields below may be left blank for DNS polling sources (i.e. <tt>dnssec:&lt;...&gt;</tt>)</i><p/>"
               "Scraper parameter: <input type=\"text\" name=\"scraperparam\"/><br/>"
//               "Scraper ID: <input type=\"text\" name=\"scraperid\"/><br/>"
               "Scraper ID: <select name=\"scraperid\"><option value=\"\"></option><option value=\"");
  appendOutput(DNSKEY_DEFAULT_DNS_SCRAPER_ID);
  appendOutput("\">DNSSEC</option><option value=\"");
  appendOutput(DNSKEY_DEFAULT_SECSPIDER_SCRAPER_ID);
  appendOutput("\">SecSpider Web page Scraper</option><option value=\"");
  appendOutput(DNSKEY_DEFAULT_RIPE_SCRAPER_ID);
  appendOutput("\">RIPE TAR</option></select><br/>"
               "<font size=\"+1\">- OR -</font><br/>"
               "Scraper (in perl)<br/><textarea name=\"scraper\" rows=\"20\" cols=\"80\"></textarea><br/>"
	             "<input type=\"submit\" value=\"Insert\"></form>");
  appendOutput("<hr/><i><a href='");
  appendOutput(getHttpPath());
  appendOutput("'>Main</a></i>");
  appendOutput("</body></html>");
  return true;
}

bool DnskeyAdminCtx::pollPage(PsUrlParser &p_oParser)
{
  appendOutput("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
  appendOutput("<html><head><title>Trigger Poll</title></head><body>Trigger poll:<p/> ");
  appendOutput("<form method=\"POST\">Enter name or URL of zone to poll immediately (leave both blank to poll all zones): <br/>"
               "<input type=\"hidden\" name=\"cmd\" value=\"poll\">"
               "Name of zone: <input type=\"text\" name=\"name\"/><br/>"
               "URL of zone: <input type=\"text\" name=\"url\"/><br/>"
	             "<input type=\"submit\" value=\"Insert\"></form>");
  appendOutput("<hr/><i><a href='");
  appendOutput(getHttpPath());
  appendOutput("'>Main</a></i>");
  appendOutput("</body></html>");
  return true;
}

bool DnskeyAdminCtx::friendPage(PsUrlParser &p_oParser)
{
  appendOutput("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
  appendOutput("<html><head><title>Friend Page</title></head><body>Vantage daemon friends:<p/> ");
  appendOutput("<form method=\"POST\">Add friend by specifying remote Vantage daemon's URL: <br/>"
               "<input type=\"hidden\" name=\"cmd\" value=\"add-friend\">"
               "User-friendly name of friend:  <input type=\"text\" name=\"name\"/><br/>"
               "URL of friend's Vantage daemon: <input type=\"text\" name=\"url\"/><br/>"
               "Public GPG key of friend's Vantage daemon:<br/><textarea name=\"pub_key\" rows=\"20\" cols=\"80\"></textarea><br/>"
               "<input type=\"submit\" value=\"Add\"></form>");
  appendOutput("<hr/>");
  appendOutput("<font size='+1'>Current Friends:<br/>");

  DaoList_t oFriendList;
  FriendDao *pDao = static_cast<FriendDao *>(DaoFactory::getInstance().create(FriendDao::s_kszDaoName));
  if (NULL == pDao)
  {
    ps_elog(PSL_CRITICAL, "Unable to create friend DAO.\n");
    appendOutput("<font color='red' size='+1'>Internal Error generating friend list.</font><br/>");
  }
  else if (!pDao->deserialize(oFriendList))
  {
    ps_elog(PSL_CRITICAL, "Unable to deserialize friend DAOs.\n");
    appendOutput("<font color='red' size='+1'>Internal Error generating friend list.</font><br/>");
  }
  else
  {
    appendOutput("<table border='0' width='100%'>");
    appendOutput("<tr align='center' bgcolor='grey'><th>URL</th><th>Name</th><th>Public Key</th></tr>");
    for (DaoIter_t tIter = oFriendList.begin();
         oFriendList.end() != tIter;
         tIter++)
    {
      FriendDao *pFriend = static_cast<FriendDao *>(*tIter);
      string sKey = crToBr(pFriend->getKey());
      appendOutput("<tr align='center'><td>");
      appendOutput(pFriend->getURL());
      appendOutput("<br/><font size='-2'>[<a href='");
      appendOutput(getHttpPath());
      appendOutput("?cmd=rem-friend&id=");
      appendOutput(pFriend->getFriendID());
      appendOutput("'>Remove</a>]</font>");
      appendOutput("</td><td>");
      appendOutput(pFriend->getName());
      appendOutput("</td><td align='left'><tt>");
      appendOutput(sKey);
      appendOutput("</tt></td></tr>");
    }
    appendOutput("</table>");
  }

  PsDao::clearList(oFriendList);
  if (NULL != pDao)
  {
    delete pDao;
    pDao = NULL;
  }

  appendOutput("<hr/><i><a href='");
  appendOutput(getHttpPath());
  appendOutput("'>Main</a></i>");
  appendOutput("</body></html>");
  return true;
}

bool DnskeyAdminCtx::addFriend(PsUrlParser &p_oParser)
{
  bool bRet = true;

  string sNameKey = "name";
  string sUrlKey = "url";
  string sPubKeyKey = "pub_key";
  string sName;
  string sUrl;
  string sPubKey;
  FriendDao *pFriend = NULL;

  if (!p_oParser.getParam(sNameKey, sName))
  {
    ps_elog(PSL_CRITICAL, "Data name not specified.\n");
    appendOutput("HTTP/1.1 500 OK\r\nContent-Type: text/html\r\n\r\n"
                 "<html><body>Data name not specified.\n</body></html>");
  }
  else if (!p_oParser.getParam(sUrlKey, sUrl))
  {
    ps_elog(PSL_CRITICAL, "Data URL not specified.\n");
    appendOutput("HTTP/1.1 500 OK\r\nContent-Type: text/html\r\n\r\n"
                 "<html><body>Data URL not specified.\n</body></html>");
  }
  else if (!p_oParser.getParam(sPubKeyKey, sPubKey))
  {
    ps_elog(PSL_CRITICAL, "Public key not specified.\n");
    appendOutput("HTTP/1.1 500 OK\r\nContent-Type: text/html\r\n\r\n"
                 "<html><body>Public key not specified.\n</body></html>");
  }
  else if ((sUrl = HttpQuery::urlDecode(sUrl)) == "")
  {
    ps_elog(PSL_CRITICAL, "Unable to parse input URL '%s'.\n", sUrl.c_str());
    appendOutput("HTTP/1.1 500 OK\r\nContent-Type: text/html\r\n\r\n"
                 "<html><body>Malformed URL.\n</body></html>");
  }
  else if ((sName = HttpQuery::urlDecode(sName)) == "")
  {
    ps_elog(PSL_CRITICAL, "Unable to parse input Name '%s'.\n", sUrl.c_str());
    appendOutput("HTTP/1.1 500 OK\r\nContent-Type: text/html\r\n\r\n"
                 "<html><body>Malformed Name.\n</body></html>");
  }
  else if ((sPubKey = HttpQuery::urlDecode(sPubKey)) == "")
  {
    ps_elog(PSL_CRITICAL, "Unable to parse input PubKey '%s'.\n", sUrl.c_str());
    appendOutput("HTTP/1.1 500 OK\r\nContent-Type: text/html\r\n\r\n"
                 "<html><body>Malformed PubKey.\n</body></html>");
  }
  else if (NULL == (pFriend = static_cast<FriendDao *>(DaoFactory::getInstance().create(FriendDao::s_kszDaoName))))
  {
    ps_elog(PSL_CRITICAL, "Unable to Create FriendDao.\n");
    appendOutput("HTTP/1.1 500 OK\r\nContent-Type: text/html\r\n\r\n"
                 "<html><body>Internal Error.\n</body></html>");
  }
  else
  {
    pFriend->setURL(sUrl);
    pFriend->setName(sName);
    pFriend->setKey(sPubKey);

    GpgmeCryptMgr *pMgr = static_cast<GpgmeCryptMgr *>(m_oApp.getCryptMgr());
    if (NULL == pMgr)
    {
      ps_elog(PSL_CRITICAL, "GPG crypto mgr is not set... aborting add.\n");
      appendOutput("HTTP/1.1 500 OK\r\nContent-Type: text/html\r\n\r\n"
                   "<html><body>Internal Error.\n</body></html>");
    }
    else if (!pFriend->serialize())
    {
      ps_elog(PSL_ERROR, "Unable to serialize friend.\n");
      appendOutput("HTTP/1.1 500 OK\r\nContent-Type: text/html\r\n\r\n"
                   "<html><body>Internal Error.\n</body></html>");
    }
    else
    {
      bool bAddedKey = pMgr->addPubKey(sPubKey);

      sPubKey = crToBr(sPubKey);
      appendOutput("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
      appendOutput("<html><body>Added new friend<p/> ");

      if (!bAddedKey)
      {
        appendOutput("<font size='+1' color='red'>Possible problem adding friend... In DB now, but may not be in crypt mgr.</font><p/>\n");
      }
      appendOutput("<table border='0' width='100%'>");
      appendOutput("<tr align='center' bgcolor='grey'><th>URL</th><th>Name</th><th>Public Key</th></tr>");
      appendOutput("<tr><td>");
      appendOutput(sUrl);
      appendOutput("</td><td>");
      appendOutput(sName);
      appendOutput("</td><td align='left'><tt>");
      appendOutput(sPubKey);
      appendOutput("</tt></td></tr>\n");
      appendOutput("</table>");
      appendOutput("<hr/><i><a href='");
      appendOutput(getHttpPath());
      appendOutput("'>Main</a></i>");
      appendOutput("</body></html>\n");
      bRet = true;
    }

    delete pFriend;
  }

  return bRet;
}

bool DnskeyAdminCtx::remFriend(PsUrlParser &p_oParser)
{
  bool bRet = true;

  string sIdKey = "id";
  string sID;
  FriendDao *pFriend = NULL;
  int iID = -1;

  if (!p_oParser.getParam(sIdKey, sID))
  {
    ps_elog(PSL_CRITICAL, "Friend ID not specified.\n");
    appendOutput("HTTP/1.1 500 OK\r\nContent-Type: text/html\r\n\r\n"
                 "<html><body>Friend ID not specified.\n</body></html>");
  }
  else if ((sID = HttpQuery::urlDecode(sID)) == "")
  {
    ps_elog(PSL_CRITICAL, "Unable to parse input ID '%s'.\n", sID.c_str());
    appendOutput("HTTP/1.1 500 OK\r\nContent-Type: text/html\r\n\r\n"
                 "<html><body>Malformed ID.\n</body></html>");
  }
  else if (0 == (iID = (int) strtol(sID.c_str(), NULL, 10)))
  {
    ps_elog(PSL_CRITICAL, "Unable to convert ID: '%s'.\n", sID.c_str());
    appendOutput("HTTP/1.1 500 OK\r\nContent-Type: text/html\r\n\r\n"
                 "<html><body>Internal Error.\n</body></html>");
  }
  else if (NULL == (pFriend = static_cast<FriendDao *>(DaoFactory::getInstance().create(FriendDao::s_kszDaoName))))
  {
    ps_elog(PSL_CRITICAL, "Unable to Create FriendDao.\n");
    appendOutput("HTTP/1.1 500 OK\r\nContent-Type: text/html\r\n\r\n"
                 "<html><body>Internal Error.\n</body></html>");
  }
  else
  {
    pFriend->setFriendID(iID);
    pFriend->setDelete(true);

    if (!pFriend->serialize())
    {
      ps_elog(PSL_ERROR, "Unable to delete friend.\n");
      appendOutput("HTTP/1.1 500 OK\r\nContent-Type: text/html\r\n\r\n"
                   "<html><body>Internal Error.\n</body></html>");
    }
    else
    {
      appendOutput("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
      appendOutput("<html><body>Deleted friend<p/> ");
      appendOutput("<hr/><i><a href='");
      appendOutput(getHttpPath());
      appendOutput("'>Main</a></i>");
      appendOutput("</body></html>\n");
      bRet = true;
    }

    delete pFriend;
  }

  return bRet;
}

bool DnskeyAdminCtx::monPage(PsUrlParser &p_oParser)
{
  appendOutput("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
  appendOutput("<html><head><title>URLs Being Monitored</title></head><body>");
  appendOutput("<font size='+1'>URLs being monitored:<br/>");

  DaoList_t oMonList;
  DnskeyMonDao *pDao = static_cast<DnskeyMonDao *>(DaoFactory::getInstance().create(DnskeyMonDao::s_kszDaoName));
  if (NULL == pDao)
  {
    ps_elog(PSL_CRITICAL, "Unable to create MonList DAO.\n");
    appendOutput("<font color='red' size='+1'>Internal Error generating list.</font><br/>");
  }
  else
  {
    int iStart = 0;
    int iStop = 30;
    string sRange;
    if (p_oParser.getParam("range", sRange))
    {
      size_t uPos = sRange.find("-");
      if (string::npos == uPos)
      {
        ps_elog(PSL_ERROR, "Could not parse: '%s'\n", sRange.c_str());
      }
      else
      {
        int iTmpStart = (int) strtol(sRange.substr(0, uPos).c_str(), NULL, 10);
        int iTmpStop = (int) strtol(sRange.substr(uPos + 1).c_str(), NULL, 10);

        if (iTmpStop > 30)
        {
          ps_elog(PSL_ERROR, "Unable to select too many rows at once: '%d'\n", iTmpStop);
        }
        else
        {
          iStart = iTmpStart;
          iStop = iTmpStop;
        }
      }
    }

    pDao->setRange(iStart, iStop);

    if (!pDao->deserialize(oMonList))
    {
      ps_elog(PSL_CRITICAL, "Unable to deserialize mon list DAOs.\n");
      appendOutput("<font color='red' size='+1'>Internal Error generating URL list.</font><br/>");
    }
    else
    {
      int iMaxID = -1;
      appendOutput("<table border='0' width='100%'>");
      appendOutput("<tr align='center' bgcolor='grey'><th>URL</th><th>Name</th><th>Scraper ID</th><th>First Seen</th><th>Last Seen</th><th>Next Poll</th></tr>");
      for (DaoIter_t tIter = oMonList.begin();
           oMonList.end() != tIter;
           tIter++)
      {
        DnskeyMonDao *pMon = static_cast<DnskeyMonDao *>(*tIter);
        iMaxID = (pMon->getID() > iMaxID) ? pMon->getID() : iMaxID;

        appendOutput("<tr align='left'><td><a href='");
        appendOutput(getHttpPath());
        appendOutput("?cmd=lookup&name=");
        appendOutput(pMon->getName());
        appendOutput("&type=raw'>");
        appendOutput(pMon->getURL());
        appendOutput("</a></td><td>");
        appendOutput(pMon->getName());
        appendOutput("</td><td>");
        appendOutput(pMon->getScraperID());
        appendOutput("</td><td>");
        appendOutput(pMon->getFirstPoll());
        appendOutput("</td><td>");
        appendOutput(pMon->getLastPoll());
        appendOutput("</td><td>");
        appendOutput(pMon->getNextPoll());
        appendOutput("</td></tr>");
      }
      appendOutput("</table><p/><a href='");
      appendOutput(getHttpPath());
      appendOutput("?sub=monlist&range=");
      appendOutput(iMaxID + 1);
      appendOutput("-");
      appendOutput(30);
      appendOutput("'>Next</a>");
    }
  }

  PsDao::clearList(oMonList);
  if (NULL != pDao)
  {
    delete pDao;
    pDao = NULL;
  }

  appendOutput("<hr/><i><a href='");
  appendOutput(getHttpPath());
  appendOutput("'>Main</a></i>");
  appendOutput("</body></html>");
  return true;
}

bool DnskeyAdminCtx::taFilePage(PsUrlParser &p_oParser)
{
  appendOutput("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
  appendOutput("<html><head><title>Trust-Anchor File Policy Page</title></head><body>");
  appendOutput("<font size='+1'>Trigger poll:</font><p/> ");
  appendOutput("On this page, you will tell Vantages what type of resolver you are using so it can create the right trust-anchor file for you.<p/>");
  appendOutput("<form method=\"POST\">Choose which type of keys to add to your trust-anchor file: <br/>"
               "<input type=\"hidden\" name=\"cmd\" value=\"ta-file\">"
               "<input type=\"radio\" name=\"policy\" value=\"unbound\"/>: Unbound file (unbound is very resilient and able to support more types of <tt>DNSKEY</tt>s)<br/>"
               "<input type=\"radio\" name=\"policy\" value=\"bind\"/>: BIND file (most versions of bind do not function well with newer key algos).<br/>"
	             "<input type=\"submit\" value=\"Set\"></form>");
  appendOutput("Note: If you are unsure, the unbound setting is more liberal about what key algorithms it will use, and the bind setting is conservative.<br/>");
  appendOutput("<hr/><i><a href='");
  appendOutput(getHttpPath());
  appendOutput("'>Main</a></i>");
  appendOutput("</body></html>");
  return true;
}

bool DnskeyAdminCtx::setTaPolicy(PsUrlParser &p_oParser)
{
  bool bRet = false;

  string sPolicyKey = "policy";
  string sPolicy;

  p_oParser.getParam(sPolicyKey, sPolicy);

  PsPolicyDao *pDao = static_cast<PsPolicyDao *>(DaoFactory::getInstance().create(PsPolicyDao::s_kszDaoName));
  if (NULL == pDao)
  {
    ps_elog(PSL_CRITICAL, "Unable to get PsPolicyDao.\n");
    appendOutput("HTTP/1.1 500 OK\r\nContent-Type: text/html\r\n\r\n"
                 "<html><body>Internal server error\n</body></html>");
  }
  else
  {
    bool bErr = false;

    sPolicy = HttpQuery::urlDecode(sPolicy);

    pDao->setType(DNSKEY_TA_FILE_POLICY_ID);
    if (sPolicy == "unbound")
    {
      pDao->addDefaultRule(PS_POLICY_ALLOW);
    }
    else if (sPolicy == "bind")
    {
      pDao->addRule("1", PS_POLICY_ALLOW);
      pDao->addRule("2", PS_POLICY_ALLOW);
      pDao->addRule("3", PS_POLICY_ALLOW);
      pDao->addRule("5", PS_POLICY_ALLOW);
      pDao->addDefaultRule(PS_POLICY_DENY);
    }
    else
    {
      ps_elog(PSL_ERROR, "Unrecognized option: '%s'.\n", sPolicy.c_str());
      bErr = true;
    }

    if (!bErr && !pDao->serialize())
    {
      ps_elog(PSL_ERROR, "Unable to serialzie TA policy.\n");
      bErr = true;
    }

    bRet = !bErr;
  }

  if (!bRet)
  {
    appendOutput("HTTP/1.1 500 OK\r\nContent-Type: text/html\r\n\r\n"
                 "<html><body>Internal server error.\n</body></html>");
  }
  else
  {
    appendOutput("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
    appendOutput("<html><head><title>Set Trust-Anchor File Policu</title></head><body>");
    appendOutput("<font size='+1'>Set Trust-Anchor File Policy to: <b>");
    appendOutput(sPolicy);
    appendOutput("</b>");
    appendOutput("<hr/><i><a href='");
    appendOutput(getHttpPath());
    appendOutput("'>Main</a></i>");
    appendOutput("</body></html>");
  }

  if (NULL != pDao)
  {
    delete pDao;
    pDao = NULL;
  }


  return true;
}
