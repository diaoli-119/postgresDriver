#pragma once

float byteToFloat(unsigned char* byteArry);

int byteToInt(unsigned char* byteArry);

int hexStrToInt(char s[]);

void genRandomStr(char *s, const int len);

bool connectToPostgres();

bool readFile();