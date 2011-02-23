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
#include <iostream>
#include <list>
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include "dns_compression.h"
#include "dns_name.h"

using namespace std;

size_t DnsCompression::add_name(DnsName &name, size_t offset, uint16_t &ptr) {
    list<string *> &p = name.m_parts;

    for (size_t i = p.size(); i > 0; --i) {
        string res;
        int skip;
        list_to_string(p, i, res, skip);

        if (locate(res, ptr))
            return p.size() - i;
        else
            m_names[res] = offset + skip;
    }

    return p.size();
}

void DnsCompression::clear()
{
  m_names.clear();
}

void DnsCompression::list_to_string(list<string *> &parts, int num,
        string &res, int &skip) {
    res.clear();
    list<string *>::reverse_iterator i = parts.rbegin();

    /* get the last num parts */
    for (int j = 0; j < num && i != parts.rend(); ++j, ++i)
        res += "." + **i;

    /* include the length of the first parts as the skip */
    for (skip = 0; i != parts.rend(); ++i)
        skip += (*i)->size() + 1;
}

bool DnsCompression::locate(string &s, uint16_t &r) {
    map<string, uint16_t>::iterator i = m_names.find(s);
    if (i != m_names.end()) {
        r = i->second;
        return true;
    }
    return false;
}

void DnsCompression::dump(void) {
    map<string, uint16_t>::iterator i;

    for (i = m_names.begin(); i != m_names.end(); ++i)
        cout << i->first << " at offset " << i->second << endl;
}
