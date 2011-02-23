#include "CCNPeerMap.h"
#include "CCNRegMsg.h"
#include <time.h>
#include <pthread.h>
#include <iostream>
using namespace std;

pthread_mutex_t mapMutex = PTHREAD_MUTEX_INITIALIZER;

void CCNPeerMap::addDiffEntry(string key, const CCNRegEntry& regEntry)
{
	pthread_mutex_lock(&mapMutex);
	m_entryMap[key] = regEntry;	
	m_entryMap[key].setLastSeen(time(NULL));
	pthread_mutex_unlock(&mapMutex);
}

void CCNPeerMap::checkTimeOut()
{
	time_t now = time(NULL);
	map<string,CCNRegEntry>::iterator it;

	//check expired entries and store their keys in expiredKeys
	vector<string> expiredKeys;
	for(it = m_entryMap.begin(); it != m_entryMap.end(); it++)
	{
		if(now - (it->second).getLastSeen() >= (it->second).getTTL())	
		{
			expiredKeys.push_back(it->first);
		}
	}

	//remove expired entries from m_entryMap
	vector<string>::iterator keyIt;
	pthread_mutex_lock(&mapMutex);
	for(keyIt = expiredKeys.begin(); keyIt != expiredKeys.end(); keyIt++)
	{
		m_entryMap.erase(*keyIt);	
	}
	pthread_mutex_unlock(&mapMutex);
}

// get (peer set Map - peer set otherThan) that contains size members
vector<CCNRegEntry> CCNPeerMap::getMap(const CCNRegMsg& otherThan, int size)
{
	vector<CCNRegEntry> sampleMap;
	map<string, CCNRegEntry>::iterator it;
		
	int filled = 0;
	for(it = m_entryMap.begin(); it != m_entryMap.end() && filled <= size; it++, filled++)
	{
		if(!otherThan.hasEntry(it->second))
		{
			sampleMap.push_back(it->second);
		}
	}

	return sampleMap;
}

void CCNPeerMap::print() const
{
	cout<<"Map size:"<<m_entryMap.size()<<endl;
	map<string, CCNRegEntry>::const_iterator it;
	for(it = m_entryMap.begin(); it != m_entryMap.end(); it++)
	{
		cout<<it->second.toString()<<endl;
	}
}
