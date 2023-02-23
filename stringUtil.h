#ifndef STRINGUTIL
#define STRINGUTIL

#include <string>
#include <vector>

const std::vector<std::string> split(const std::string& str, const std::string& pattern);
std::string def_color = "\033[0m";
std::string colors[] = {"\033[31m", "\033[32m", "\033[33m", "\033[34m", "\033[35m", "\033[36m"}; // 根據client id來選擇color -> id % NUM_COLORS

#endif