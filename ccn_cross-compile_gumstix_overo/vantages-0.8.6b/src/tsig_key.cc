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
#include <errno.h>
#include <string.h>

#include "tsig_key.h"
#include "dns_defs.h"
#include "b64.h"

TsigKey::TsigKey(std::string &name) : m_init(false), m_name(name) {
}

bool TsigKey::load(char *file) {
    FILE *f = NULL;
    char buf[BUFSIZ];
    int key_len;

    if ((f = fopen(file, "r")) == NULL) {
        dns_log("Couldn't open TSIG key file '%s': %s\n", file, strerror(errno));
        goto error;
    }

    if (fgets(buf, BUFSIZ, f) == NULL || fgets(buf, BUFSIZ, f) == NULL) {
        dns_log("TSIG key file '%s' is empty\n", file);
        goto error;
    }

    if (strncmp(buf, "Algorithm: 157", 14) != 0) {
        dns_log("TSIG key algorithm is not HMAC-MD5\n");
        goto error;
    }

    if (fgets(buf, BUFSIZ, f) == NULL) {
        dns_log("TSIG key file does not contain a key\n");
        goto error;
    }

    // decode the key
    key_len = b64_decode((u_char *)(buf + 4), m_key, strlen(buf + 4), MAX_KEYLEN);

    if (key_len < 0) {
        dns_log("TSIG key is too long (max supported: %d bytes)\n", MAX_KEYLEN);
        goto error;
    }
    m_key_length = key_len - 1;

    m_init = true;
    return true;

error:
    if (f != NULL)
        fclose(f);
    m_init = false;
    return false;
}
