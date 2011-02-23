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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <string>

#include "ps_policy_dao.h"
#include "ps_logger.h"
#include "ps_defs.h"

using namespace std;

void _usage()
{
  fprintf(stdout, "test_ps_policy_dao <DB cxn> [ <policy ID> ] | -h\n");
}

int main(int argc, char *argv[])
{
  int iRet = 1;

  if (argc < 2)
  {
    cerr << "Must specify DB" << endl;
    _usage();
  }
  else if (0 == strcmp("-h", argv[1]))
  {
    _usage();
  }
  else
  {
    string sCxn = argv[1];
    PsDao::setConnectString(sCxn);

    PsLogger::getInstance().setLevel(PSL_DEBUG);

    int iType = 1;
    if (argc > 2)
    {
      iType = (int) strtol(argv[2], NULL, 10);
    }

    cout << "Using type: " << iType << endl;

    PsPolicyDao oDao;
    oDao.setType(iType);

    if (!oDao.addRule("a", PS_POLICY_ALLOW))
    {
      cerr << "Unable to add policy: 'a' -> " << PS_POLICY_ALLOW << endl;
    }
    else if (!oDao.addRule("b", PS_POLICY_DENY))
    {
      cerr << "Unable to add policy: 'b' -> " << PS_POLICY_DENY << endl;
    }
    else if (oDao.addRule("c", PS_POLICY_UNKNOWN))
    {
      cerr << "WAS able to add policy: 'c' -> " << PS_POLICY_UNKNOWN << endl;
    }
    else if (!oDao.addDefaultRule(PS_POLICY_DENY))
    {
      cerr << "Unable to add default plicy '' -> " << PS_POLICY_DENY << endl;
    }
    else if (!oDao.serialize())
    {
      cerr << "Unable to serialize" << endl;
    }
    else if (!oDao.deserialize())
    {
      cerr << "Unable to deserialize" << endl;
    }
    else if (PS_POLICY_ALLOW != oDao.check("a"))
    {
      cerr << "Was not able to get an ALLOW for 'a'" << endl;
    }
    else if (PS_POLICY_DENY != oDao.check("b"))
    {
      cerr << "Was not able to get an DENNY for 'b'" << endl;
    }
    else if (PS_POLICY_DENY != oDao.check("c"))
    {
      cerr << "Was not able to get default DENNY for 'c'" << endl;
    }
    else if (PS_POLICY_DENY != oDao.check("sdkjfhksd"))
    {
      cerr << "Was not able to get default DENNY for 'sdkjfhksd'" << endl;
    }
    else
    {
      iRet = 0;
    }

    oDao.print();
  }

  if (0 == iRet)
  {
    cout << ">>>SUCCESS<<<" << endl;
  }
  else
  {
    cout << ">>>FAIL<<<" << endl;
  }

  return iRet;
}
