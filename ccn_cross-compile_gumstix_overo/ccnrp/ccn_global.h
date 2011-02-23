#ifndef _CCN_GLOBAL_H
#define _CCN_GLOBAL_H

#include <stdlib.h>
#include <string.h>
#include <string>
#include <iostream>
using namespace std;
#define MAXBUFLEN 1000
void mempush(char** s1, const void* s2, size_t n);
void mempop(void* s1, char** s2, size_t n);
int proto2n(string proto);
string n2proto(int n);

#endif
