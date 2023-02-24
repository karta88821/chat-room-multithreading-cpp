#include "stringUtil.h"
#include <string.h>

using namespace std;

const vector<string> split(const string& str, const string& pattern) {
    vector<string> result;
    string::size_type begin, end;

    end = str.find(pattern);
    begin = 0;

    while (end != string::npos) {
        if (end - begin != 0) {
            result.push_back(str.substr(begin, end-begin)); 
        }    
        begin = end + pattern.size();
        end = str.find(pattern, begin);
    }

    if (begin != str.length()) {
        result.push_back(str.substr(begin));
    }
    return result;        
}

void combine(char *res, const char *pattern, vector<const char *> strs)
{
	memset(res, 0, MAX_LEN);

	for (int i = 0; i < strs.size(); i++)
	{
		strcat(res, strs[i]);
		if (i != strs.size() - 1)
		{
			strcat(res, pattern);
		}
	}
}
