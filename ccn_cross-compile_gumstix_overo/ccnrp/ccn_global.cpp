#include "ccn_global.h"

void mempush(char** s1, const void* s2, size_t n)
{
    memcpy(*s1, s2, n);
    *s1 += n;
}

void mempop(void* s1, char** s2, size_t n)
{
	memcpy(s1, *s2, n);
	*s2 += n;
}

int proto2n(string proto)
{
    if(proto == "tcp")
    {   
        return 6;
    }else if(proto == "udp")
    {   
        return 17; 
    }   
    
    cerr<<"Unrecognized protocol"<<endl;
    return -1;
}

string n2proto(int n)
{
	if(n == 6)
	{
		return "tcp";
	}else if(n == 17)
	{
		return "udp";
	}
	
	cerr<<"Unrecognized n: " << n <<endl;
	return "";
}

