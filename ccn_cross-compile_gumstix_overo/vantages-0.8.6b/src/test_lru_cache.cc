#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <sstream>

#include "lru_cache.h"

using namespace std;

int main(int argc, char *argv[])
{
  LruCache<string, string> oCache;

  for (int i = 0; i < 1000; i++)
  {
    stringstream oSS;
    oSS << (i % 10);
    string sKey = oSS.str();
    if (!oCache.get(sKey))
    {
      string *s = new string(sKey);
      oCache.add(sKey, *s);
    }

    oSS.clear();
    oSS << (rand() % 1000);
    sKey = oSS.str();
    oCache.get(sKey);

    oSS.clear();
  }

  return 0;
}
