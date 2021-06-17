#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

static long int count = 1;

float byteToFloat(unsigned char* byteArry) { return *((float*)byteArry); }

int byteToInt(unsigned char* byteArry) { return *((int*)byteArry); }

int hexStrToInt(char s[])
{
	int i = 0;
	int n = 0;
	if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) i = 2;
	else i = 0;

	for (; (s[i] >= '0' && s[i] <= '9') || (s[i] >= 'a' && s[i] <= 'z') || (s[i] >= 'A' && s[i] <= 'Z'); ++i)
	{
		if (tolower(s[i]) > '9') n = 16 * n + (10 + tolower(s[i]) - 'a');
		else n = 16 * n + (tolower(s[i]) - '0');\
	}
	return n;
}

void genRandomStr(char *s, const int len)
{
	srand((count++) * time(0));
	const char rawStr[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890";
	for (int i = 0; i < len; ++i)
	{
		s[i] = rawStr[rand() % (sizeof(rawStr) - 1)];
	}
	s[len - 1] = 0;
}

time_t StringToDatetime(char timeStr[])
{
	struct tm tmIns;
	uint32_t year, month, day, hour, min, sec;
	sscanf(timeStr, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &min, &sec);
	tmIns.tm_year = year - 1900;	//year starts from 1900 in struct tm
	tmIns.tm_mon = month - 1;	//month: 0 - 11
	tmIns.tm_mday = day;
	tmIns.tm_hour = hour;
	tmIns.tm_min = min;
	tmIns.tm_sec = sec;
	tmIns.tm_isdst = 0;			//un-saving-time
	return mktime(&tmIns);
}