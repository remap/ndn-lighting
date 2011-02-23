#ifndef _CCNRP_DEFS_H
#define _CCNRP_DEFS_H

#include <inttypes.h>

typedef struct 
{
  uint8_t m_uVersion;
  uint8_t m_uType;
  uint16_t m_uLen;
} __attribute__((__packed__)) ccnrp_header_t;

typedef struct 
{
  uint32_t m_uIP;
  uint16_t m_uPort;
  uint16_t m_uTTL;
  uint8_t m_uProto;
  uint8_t m_uNsLen;
  const char m_pNS[0];
} __attribute__((__packed__)) ccnrp_peer_msg_t;

#endif
