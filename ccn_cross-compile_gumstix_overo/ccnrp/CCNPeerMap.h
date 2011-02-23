#ifndef _CCN_PEERMAP_H
#define _CCN_PEERMAP_H
#include <map>
#include <string>
#include <vector>
#include "CCNRegEntry.h"
using namespace std;

#define PEERLIST_REPLY_SIZE 5
class CCNPeerMap
{
private:
	map<string, CCNRegEntry> m_entryMap;
public:
	void addDiffEntry(string key, const CCNRegEntry& regEntry);
	vector<CCNRegEntry> getMap(const CCNRegMsg& otherThan, int size=PEERLIST_REPLY_SIZE);
	void checkTimeOut();
	void print() const;
};
#endif
