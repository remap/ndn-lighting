#ifndef _CCN_REG_MSG_H
#define _CCN_REG_MSG_H

#include <netinet/in.h>
#include <stdint.h>
#include "CCNRegEntry.h"
#include "CCNPeerMap.h"
#include <map>
#include <string>
using namespace std;

// #define MAX_ENTRY_NUM 10
#define T_REGISTRATION 1
#define T_PEERLIST_REPLY 2
#define VERSION 1
#define CCNRP_MAX_PACKET_SIZE 4096

class CCNRegMsg
{
private:
	uint8_t m_uVersion;
	uint8_t m_uMsgType;
	uint16_t m_uMsgLen; //length the whole message
	map<string, CCNRegEntry> m_entryMap;

public:
	CCNRegMsg(uint8_t _type, uint8_t version = VERSION);	
	CCNRegMsg(const CCNRegMsg &p_oRHS);
	virtual ~CCNRegMsg();

  CCNRegMsg &operator=(const CCNRegMsg &p_oRHS);

  bool extractMsg(int p_iSocket, struct sockaddr_in &p_tAddr);
//	void extractMsg(char* msg);	
	void addEntry(const CCNRegEntry& _entry);
	bool hasEntry(const CCNRegEntry& _entry) const;
	int getLength();
	int getEntryNum();
	map<string,CCNRegEntry>::iterator begin();
	map<string,CCNRegEntry>::iterator end();
	void getMsg(char** message);
  bool toWire(char *p_pBuff, size_t p_uLen);
  bool sendMsg(int p_iSocket, struct sockaddr_in &p_tAddr);
	void print();

	//interaction with peermap
	void registerMsg(CCNPeerMap& peerMap);
};
#endif
