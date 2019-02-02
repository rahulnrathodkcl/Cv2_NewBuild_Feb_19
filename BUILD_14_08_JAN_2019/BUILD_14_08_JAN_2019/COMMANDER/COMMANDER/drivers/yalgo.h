
#ifndef YALGO_H_
#define YALGO_H_
#include <inttypes.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>

void StringtoUpperCase(char *str);
bool StringstartsWith(const char *str,const char *pre);
void subString(const char* input, int offset, int len, char* dest);

#endif /* YALGO_H_ */