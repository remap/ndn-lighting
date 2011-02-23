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
#include <string.h>

#include <string>

#include "ps_config.h"
#include "config.h"

using namespace std;

PsConfig PsConfig::s_oInstance;

/*
 * Method: PsConfig()
 *
 * Purpose:
 *    Constructor.
 *
 */
PsConfig::PsConfig()
  : m_szFile(NULL),
    m_szError(NULL)
{

}

/*
 * Method: ~PsConfig()
 *
 * Purpose:
 *    Destructor.
 *
 */
PsConfig::~PsConfig()
{
  if (NULL != m_szFile)
  {
    delete[] m_szFile;
    m_szFile = NULL;
  }
}

PsConfig &PsConfig::getInstance()
{
  return s_oInstance;
}

void PsConfig::clear()
{
  m_oConfigMap.clear();
}

/*
 * Method: load()
 *
 * Purpose:
 *    Given an absolute file name (with its path), this method
 *    opens the file and parses it.
 *
 */
int PsConfig::load(char *p_szFile)
{
  int iRet = 0;
  FILE *pFile = NULL;

  // Make sure the user specified a file.
  if (NULL == p_szFile)
  {
    m_szError = "No config file specified";
  }
  // Try to open it for reading.
  else if (NULL == (pFile = fopen(p_szFile, "r")))
  {
    m_szError = "Unable to open file";
  }
  else
  {
    // If the file was opened, copy the file parameter
    // into a member variable.
    int iLen = strlen(p_szFile);
    m_szFile = new char[iLen + 1];
    memset(m_szFile, 0, iLen + 1);
    strncpy(m_szFile, p_szFile, iLen);

    char szLine[PS_CONFIG_LINE_LEN + 1];
    char *szRet = NULL;
    memset(szLine, 0, PS_CONFIG_LINE_LEN + 1);

    clear();

    // Get the first line of the file
    while (NULL != (szRet = fgets(szLine, PS_CONFIG_LINE_LEN, pFile)))
    {
      // If this was NOT a comment line...
      if ('#' != szLine[0])
      {
        // Look for the first '=' (as the delimiter).
        char *szEq = index(szLine, '=');
        // If there was a delimiter, then this is a valid line.
        if (NULL != szEq)
        {
          // Point at the second token as the value.
          char *szValue = &(szEq[1]);
          // change the delimiter to be a NULL so we have 2 strings now (key and value).
          szEq[0] = '\0';

          // Get the string lengths.
          int iKeyLen = PS_CONFIG_LINE_LEN - (int) (szEq - szLine);
          int iValLen = PS_CONFIG_LINE_LEN - (int) (szValue - szRet);

          // Get rid of trailing spaces.
          _chomp(szValue, iValLen);

          // Trim the key and values of uneeded white space.
          char *szKey = _trim(szLine, iKeyLen);
          szValue = _trim(szValue, iValLen);

          // Add the key and value into the member map.
          m_oConfigMap[szKey].push_back(szValue);
        }
      }
      // Reset the line.
      memset(szLine, 0, PS_CONFIG_LINE_LEN + 1);
    }

    // It worked...
    iRet = 1;

    // Close the file.
    fclose(pFile);
  }

  // Return the status.
  return iRet;
}

int PsConfig::load(const char *p_szFile)
{
  return load((char *) p_szFile);
}

/*
 * Method: getValue()
 *
 * Purpose:
 *    Given a key, look it up in the member map and
 *    return the result (if any).
 *
 */
const char *PsConfig::getValue(char *p_szKey)
{
  return getValue((const char *) p_szKey);
}

/*
 * Method: getValue()
 *
 * Purpose:
 *    Given a key, look it up in the member map and
 *    return the result (if any).
 *
 */
const char *PsConfig::getValue(const char *p_szKey)
{
  const char *szRet = NULL;

  // Make sure a key was specified.
  if (NULL != p_szKey)
  {
    // Find if there is a value.
    ConfigMapIter_t tIter = m_oConfigMap.find(p_szKey);

    // If we found a value...
    if (m_oConfigMap.end() != tIter)
    {
      // Return it as a c-string.
      szRet = tIter->second.front().c_str();
    }
  }

  // Return whatever we found.
  return szRet;
}

/*
 * Function: _chomp()
 *
 * Purpose: 
 *    If this line has newline/carriage return char(s)
 *    at the end, this function will replace them with NULL.
 *
 */
void PsConfig::_chomp(char *p_szLine, int p_iMaxLen)
{
  // If we have a line...
  if (NULL != p_szLine)
  {
    int i = 0;
    // Find the length.
    for (i = 0; '\0' != p_szLine[i] && i < p_iMaxLen; i++) {}

    // If i makes sense...
    if (i < p_iMaxLen)
    {
      // Go from the end to the beginning turning \r and \n into \0 until
      // we see the first non-\r\n char.
      while (--i >= 0 && ('\n' == p_szLine[i] || '\r' == p_szLine[i]))
      {
        p_szLine[i] = '\0';
      }
    }
  }
}

/*
 * Function: _trim()
 *
 * Purpose: 
 *    A simple inline function that returns a pointer to the 
 *    input string that points passed any intial whitespaces,
 *    and has all of the trailing whitespace removed.
 *
 */
char *PsConfig::_trim(char *p_szLine, int p_iMaxLen)
{
  char *szRet = NULL;

  // If we have an input line...
  if (NULL != p_szLine)
  {
    char *pLastChar = NULL;
    int i = 0;

    // From the beginning, look for leading whitespace...
    for (i = 0; '\0' != p_szLine[i] && i < p_iMaxLen; i++)
    {
      // If we haven't found the first non-whitespace char
      // yet, and this is not whitespace...
      if (NULL == pLastChar
          && ' ' != p_szLine[i]
          && '\t' != p_szLine[i])
      {
        // Set this as the head of the trimmed string
        szRet = &(p_szLine[i]);
        pLastChar = szRet;
      }
      // If we've found the start, keep a pointer to the last
      // non-whitespace character.
      else if (' ' != p_szLine[i]
               && '\t' != p_szLine[i])
      {
        pLastChar = &(p_szLine[i]);
      }
    }

    // After the loop, if we have any trailing characters that are
    // not pointed to, then they must be whitespace, so we insert a
    // NULL to chop them off.
    if (i < p_iMaxLen)
    {
      pLastChar[1] = '\0';
    }
  }

  return szRet;
}

