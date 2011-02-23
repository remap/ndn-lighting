#ifndef _BASE_64_H
#define _BASE_64_H

#include <string>
#include <vector>

std::string base64_encode(unsigned char const* , unsigned int len);
std::vector<unsigned char> base64_decode(std::string const& s);

#endif
