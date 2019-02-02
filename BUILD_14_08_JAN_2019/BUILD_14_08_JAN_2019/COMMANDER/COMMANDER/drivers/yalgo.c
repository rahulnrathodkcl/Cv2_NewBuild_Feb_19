#include "yalgo.h"
#include "itoa.h"


void StringtoUpperCase(char *str)
{
	if (!str) return;
	for (char *p = str; *p; p++) {
		*p = toupper(*p);
	}
}

bool StringstartsWith(const char *str,const char *pre)
{
	size_t lenpre = strlen(pre),
	lenstr = strlen(str);
	return lenstr < lenpre ? false : strncmp(pre, str, lenpre) == 0;
}

void subString(const char* input, int offset, int len, char* dest)
{
	strncpy (dest, input + offset, len);
}