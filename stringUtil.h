#ifndef STRINGUTIL
#define STRINGUTIL

#include <string>
#include <vector>

#define MAX_LEN 256
#define NUM_COLORS 5

const std::vector<std::string> split(const std::string& str, const std::string& pattern);
void combine(char *res, const char *pattern, std::vector<const char *> strs);

#endif