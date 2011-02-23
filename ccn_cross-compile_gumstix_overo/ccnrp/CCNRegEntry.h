#ifndef _CCN_REG_ENTRY_H
#define _CCN_REG_ENTRY_H

#include <stdint.h>
#include <time.h>
#include <string>

#include "ccnrp_defs.h"

using namespace std;

class CCNRegMsg;

class CCNRegEntry
{
private:
/*
	char* msg; //pointer to message

public:
	uint32_t m_uIP;// network order of IP
    uint16_t m_uPort;
    uint16_t m_uTTL;
    uint8_t m_uProtocol;
    uint8_t m_uNameLen; //length of the namespace
    char* m_ccn_namespace;
*/
  ccnrp_peer_msg_t *m_pMsg;
  char *m_pBuffer;
  size_t m_uLen;
	
	time_t m_lastSeen;
	
public:
	CCNRegEntry();
	CCNRegEntry& operator = (const CCNRegEntry& regEntry);
	bool operator==(const CCNRegEntry& regEntry);
	CCNRegEntry(const CCNRegEntry& regEntry);
	virtual ~CCNRegEntry();

  int extractEntry(char *pEntry);
  int toWire(char *p_pBuff, size_t p_uLen);

  size_t getLen();

  uint32_t getIP() const;
  void setIP(uint32_t p_uIP);

  uint16_t getPort() const;
  void setPort(uint16_t p_uPort);

  uint16_t getTTL() const;
  void setTTL(uint16_t p_uTTL);

  uint8_t getProto() const;
  void setProto(uint8_t p_uProto);

  const char *getNameSpace() const;
	void setNameSpace(const char* ccnNameSpace, int length);

  uint8_t getNsLen();

  time_t getLastSeen() const;
  void setLastSeen(time_t p_tSeen);

	uint16_t getEntryLen() const;
	ccnrp_peer_msg_t *getEntry();
	string getKey() const; //the key is to be used in CCNPeerMap
	string toString() const;
};
#endif
