#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <libpq-fe.h>
#include <errno.h>
#include <stdbool.h>
#include <time.h>
#include "conf.h"
#include "pubFunc.h"

PGconn *conn;
PGresult *res;
static long int fileLen = 0;

bool convertMeshInfo(char meshInfo[], uint32_t type, char watchName[], uint16_t nodeAddr[], char createdTime[], float *ptrTemp, float *ptrHumi, 
					 float *ptrXPos, float *ptrYPos, double *ptrbTemp)
{
	uint16_t i = 0;
	
	/*Obtain created time string*/
	while ('\t' != meshInfo[i])
	{
		createdTime[i] = meshInfo[i];
		i++;
	}
	/*Convert time string to Datetime*/
	//*t_ = StringToDatetime(createdTime);
	//*t_ += 12 * 3600;

	/*jump over identifier "?"*/
	while ('?' != meshInfo[i++]);

	/*Get node addr*/
	memcpy(nodeAddr, (const char *)&meshInfo[i++], 2);

	/*jump over identifier "!"*/
	while ('!' != meshInfo[i++]);

	/*Get data length*/
	//uint16_t dataLen = meshInfo[i++];
	
	/*jump over dataLen byte*/
	i++;
	
	/*jump over "header","type"*/
	i += HEAD_LEN + TYPE_LEN;

	if ( MSG_TYPE_TH == type )		//SHT30 Temp&Humi Sensor msg. T:0x54, H:0x48, type:0x5448
	{
		/*Convert temperature*/
		char tempStr[ENV_TEMP_LEN] = { 0 };
		
		/*In case temp or huim data is empty*/
		strncpy(tempStr, &meshInfo[i], ENV_TEMP_LEN);
		
		*ptrTemp = byteToFloat((unsigned char *)tempStr);
		printf("%0.2f\n", *ptrTemp);
		
		/*jump over temp*/
		i += ENV_TEMP_LEN;

		/*Convert humidity*/
		char humiStr[ENV_HUMI_LEN] = { 0 };
		strncpy(humiStr, &meshInfo[i], ENV_HUMI_LEN);
		
		*ptrHumi = byteToFloat((unsigned char *)humiStr);
		printf("%0.2f\n", *ptrHumi);
		return true;
	}
	else if ( MSG_TYPE_PO == type )	//Position msg. P:0x50, O:0x4F, type: 0x504F
	{
		/*Convert position*/
		char posStrX[X_POS_LEN] = { 0 };
		char posStrY[Y_POS_LEN] = { 0 };

		/*In case position data is empty*/
		if ('\n' == meshInfo[i]) return false;
		strncpy(posStrX, &meshInfo[i], X_POS_LEN);
		/*jump over x*/
		i += X_POS_LEN;
		
		strncpy(posStrY, &meshInfo[i], Y_POS_LEN);
		
		*ptrXPos = byteToFloat((unsigned char *)posStrX);
		*ptrYPos = byteToFloat((unsigned char *)posStrY);
		printf("%0.2f\n", *ptrXPos);
		printf("%0.2f\n", *ptrYPos);
		return true;
	}
	else if ( MSG_TYPE_PI == type )	//PIR Sensor msg. P:0x50, I:0x4F, type: 0x5049
	{
		return true;
	}
	else if ( MSG_TYPE_BE == type )	//body temperature from watch
	{
		uint16_t nameLen = meshInfo[i++] - 1;	//name does not include 0x09, so the actual nameLen is meshInfo[i] - 1

		/*jump over 0x09 and watch name*/
		i++;

		/*Get watch name*/
		strncpy(watchName, &meshInfo[i], nameLen);

		/*jump over 0x10, 0x16, uuid: 0x03, 0x18*/
		i += nameLen + 4;
		
		/*Convert body temperature*/
		char bIntTempStr[4] = { 0 };
		char bDecTempStr[4] = { 0 }; 
		sprintf(bIntTempStr, "%d", meshInfo[i++]);
		sprintf(bDecTempStr, "%d", meshInfo[i]);
		char tempStr[9] = { 0 };	//int 4 bytes, deci 4 bytes, '.' 1 byte.
		strncpy(tempStr, bIntTempStr, strlen(bIntTempStr));
		strncat(tempStr, ".", 1);
		strncat(tempStr, bDecTempStr, strlen(bDecTempStr));
		*ptrbTemp = atof(tempStr);
		return true;
	}
	return false;
}

void insertIntoPostgres(char sqlComm[], char objId[], char createdTime[], char header[], char type[], char nodeAddr[], char watchName[], char tempStr[], 
						char humiStr[], char x[], char y[])
{
	strcat(sqlComm, "'");
	strcat(sqlComm, objId);
	strcat(sqlComm, "',");
	strcat(sqlComm, "TO_TIMESTAMP('");
	strcat(sqlComm, createdTime);
	strcat(sqlComm, "', 'yyyy-mm-dd hh24:mi:ss'),");
	strcat(sqlComm, "NOW(),'");
	strcat(sqlComm, header);
	strcat(sqlComm, "','");
	strcat(sqlComm, type);
	strcat(sqlComm, "','");
	strcat(sqlComm, nodeAddr);
	strcat(sqlComm, "'");
	
	if(!strcmp("MSG_TYPE_PO", type))
	{
		strcat(sqlComm, ",'");
		strcat(sqlComm, x);
		strcat(sqlComm, "','");
		strcat(sqlComm, y);
		strcat(sqlComm, "')");
	}
	else if(!strcmp("MSG_TYPE_TH", type))
	{
		strcat(sqlComm, ",'");
		strncat(sqlComm, tempStr, strlen(tempStr));
		strcat(sqlComm, "','");
		strncat(sqlComm, humiStr, strlen(humiStr));
		strcat(sqlComm, "')");
	}
	else if(!strcmp("MSG_TYPE_BE", type))
	{
		strcat(sqlComm, ",'");
		strcat(sqlComm, watchName);
		strcat(sqlComm, "','");
		strcat(sqlComm, tempStr);
		strcat(sqlComm, "')");
	}
	else if(!strcmp("MSG_TYPE_PI", type))
	{
		//no need to add column and data
		strcat(sqlComm, ")");
	}
	else
	{
		printf("No type matched! return");
		return;
	}
	printf("\n%s\n", sqlComm);
}

void updateDataToPsql(char header[], uint32_t type, char watchName[], uint16_t *nodeAddr, char createdTime[], float *ptrTemp, float *ptrHumi, float *ptrXPos,
					  float *ptrYPos, double *ptrbTemp)
{
	time_t timer;
	time(&timer);
	struct tm *tmNow = NULL;
	tmNow = localtime(&timer);
    printf("\nsaveToFile L152, current time is:%d-%d-%d\t%d:%d:%d\n", (1900+tmNow->tm_year), (1+tmNow->tm_mon), (tmNow->tm_mday),
            tmNow->tm_hour, tmNow->tm_min, tmNow->tm_sec);
	char currentTime[TIME_STR_LEN] = {0};
	sprintf(currentTime, "%4d%2d%2d%2d%2d%2d",  (1900+tmNow->tm_year), (1+tmNow->tm_mon), (tmNow->tm_mday), tmNow->tm_hour, tmNow->tm_min, tmNow->tm_sec);
	
	/*Generate objId*/
	char objId[11] = { 0 };
	genRandomStr(objId, 11);
	
	/*Convert nodeaddr to string*/
	char nodeAddStr[8] = {0};
	sprintf(nodeAddStr, "%d", *nodeAddr);
	
	char sqlComm[SQLCOMMAND_LEN] = {0};
	switch (type)
	{
		case MSG_TYPE_PO:
		{
			printf("type = MSG_TYPE_PO\n");
			/*Get Position*/
			/*convert x from float to string*/
			char xStr[8] = {0};
			sprintf(xStr, "%.2f", *ptrXPos);
			/*convert y from float to string*/
			char yStr[8] = {0};
			sprintf(xStr, "%.2f", *ptrYPos);
			strcpy(sqlComm, "INSERT INTO position (\"objectId\", \"createdAt\", \"updatedAt\", header, type, nodeAddr, x, y) VALUES (");
			insertIntoPostgres(sqlComm, objId, createdTime, header, "MSG_TYPE_PO", nodeAddStr, "", "" , "", xStr, yStr);			
			break;
		}
		case MSG_TYPE_TH:
		{
			printf("type = MSG_TYPE_TH\n");
			/*Get SHT30 Temo&Humi Sensor msg*/
			/*convert temperature from float to string*/
			char tempStr[8] = {0};
			sprintf(tempStr, "%.2f", *ptrTemp);
			/*convert humidity from float to string*/
			char humiStr[8] = {0};
			sprintf(humiStr, "%.2f", *ptrHumi);
			strcpy(sqlComm, "INSERT INTO temp_humi (\"objectId\", \"createdAt\", \"updatedAt\", header, type, nodeAddr, temp, humi) VALUES (");
			printf("\n%s\n\n", tempStr);
			insertIntoPostgres(sqlComm, objId, createdTime, header, "MSG_TYPE_TH", nodeAddStr, "", tempStr, humiStr, "", "");
			break;
		}
		case MSG_TYPE_BE:
		{
			printf("type = MSG_TYPE_BE\n");
			/*Get body temperature from watch*/	
			/*convert temperature from float to string*/
			char tempStr[8] = {0};
			sprintf(tempStr, "%.2f", *ptrbTemp);		
			strcpy(sqlComm, "INSERT INTO body_temp (\"objectId\", \"createdAt\", \"updatedAt\", header, type, nodeAddr, watchname, temp) VALUES (");
			insertIntoPostgres(sqlComm, objId, createdTime, header, "MSG_TYPE_BE", nodeAddStr, watchName, tempStr, "", "", "");
			break;
		}
		case MSG_TYPE_R1:
		{
			printf("type = MSG_TYPE_R1\n");
			return;
		}
		case MSG_TYPE_R2:
		{
			printf("type = MSG_TYPE_R2\n");			
			return;
		}
		case MSG_TYPE_PI:
		{
			printf("type = MSG_TYPE_PI\n");
			/*Get PIR*/		
			strcpy(sqlComm, "INSERT INTO pir (\"objectId\", \"createdAt\", \"updatedAt\", header, type, nodeaddr) VALUES (");
			insertIntoPostgres(sqlComm, objId, createdTime, header, "MSG_TYPE_PI", nodeAddStr, "", "", "", "", "");
			break;
		}
		case MSG_TYPE_SD:
		{
			printf("type = MSG_TYPE_SD\n");			
			return;
		}
		case MSG_TYPE_Alert: 
		{
			printf("type = MSG_TYPE_Alert\n");			
			return;
		}
		default:
		{
			printf("type = DEFAULT\n");			
			return;
		}
	}
	res = PQexec(conn, sqlComm);
	if (PQresultStatus(res) != PGRES_COMMAND_OK) fprintf(stderr, "insert data failed: %s\n", PQerrorMessage(conn));
	PQclear(res);
}

bool readFile()
{
	FILE *fp = fopen(MESHDATA, "rb");
	if(!fp) 
	{
		printf("open meshData.txt failed: error %d(%s)\n", errno, strerror(errno));
		return false;
	}
	float temp = 0;
	float humi = 0;
	float xPos = 0;
	float yPos = 0;
	double bTemp = 0;
	uint32_t type = 0;
	uint16_t nodeAddr = 0;
	uint16_t pos = 0;
	uint16_t i = 0;
	uint16_t item = 0;
	char buf[BUF_LEN] = {0};
	char watchName[WATCHNAME_LEN] = { 0 };
	char createdTime[TIME_STR_LEN] = {0};
	
	/*set file current position*/
	fseek(fp, fileLen, SEEK_SET);
	
	while(!feof(fp))
	{
		do
		{
			buf[i] = fgetc(fp);
			if (buf[i] == 0x0a && buf[i - 1] == 0x09 && buf[i - 2] == 0x0a) break;	//last three bytes in every line
			i++;
		} while (i < BUF_LEN);
		
		while (pos < BUF_LEN)
		{
			if ('D' == buf[pos++] && 'N' == buf[pos++])
			{
				strncpy((char *)&type, &buf[pos + 1], 1);
				strncat((char *)&type, &buf[pos], 1);
				if (convertMeshInfo(buf, type, watchName, &nodeAddr, createdTime, &temp, &humi, &xPos, &yPos, &bTemp))
				{
					updateDataToPsql(RECORD_HEADER, type, watchName, &nodeAddr, createdTime, &temp, &humi, &xPos, &yPos, &bTemp);
				}
				memset(createdTime, 0, TIME_STR_LEN);
				memset(buf, 0, BUF_LEN);
				type = 0;
				i = 0;
				pos = 0;
				item++;
				break;
			}
		}
	}
	
	/*get length of current file*/
	fileLen = ftell(fp);
	
	fclose(fp);
	fp = NULL;
	return true;
}

bool connectToPostgres()
{
	/* Connect to PostgreSQL with user:pi and database:parse. */
	conn = PQconnectdb("user=pi dbname=parse");
	if (PQstatus(conn) == CONNECTION_BAD) {
		fprintf(stderr, "Connection to database failed: %s\n", PQerrorMessage(conn));
		PQfinish(conn);
		exit(EXIT_FAILURE);
	}
#if 0
	/*Create table*/
	char *createTemp_Humi = "CREATE TABLE temp_humi (objectId VARCHAR(24), createdAt TIMESTAMPTZ, updatedAt TIMESTAMPTZ, header VARCHAR(8), type VARCHAR(16), nodeAddr VARCHAR(8), temp VARCHAR(16), humi VARCHAR(16))";
	res = PQexec(conn, createTemp_Humi);
	if (PQresultStatus(res) != PGRES_COMMAND_OK) fprintf(stderr, "Create table temp_humi failed: %s\n", PQerrorMessage(conn));
	else printf("create temp_humi successfully!\n");
	PQclear(res);
	
	char *createPosition = "CREATE TABLE position(objectId VARCHAR(24), createdAt TIMESTAMPTZ, updatedAt TIMESTAMPTZ, header VARCHAR(8), type VARCHAR(16), nodeAddr VARCHAR(8), x VARCHAR(16), y VARCHAR(16))";
	res = PQexec(conn, createPosition);
	if (PQresultStatus(res) != PGRES_COMMAND_OK) fprintf(stderr, "Create table position failed: %s\n", PQerrorMessage(conn));
	else printf("create position successfully!\n");
	PQclear(res);
	
	char *createBody_Temp = "CREATE TABLE body_temp(objectId VARCHAR(24), createdAt TIMESTAMPTZ, updatedAt TIMESTAMPTZ, header VARCHAR(8), type VARCHAR(16), nodeAddr VARCHAR(8), watchname VARCHAR(16), temp VARCHAR(16))";
	res = PQexec(conn, createBody_Temp);
	if (PQresultStatus(res) != PGRES_COMMAND_OK) fprintf(stderr, "Create table body_temp failed: %s\n", PQerrorMessage(conn));
	else printf("create body_temp successfully!\n");
	PQclear(res);
#endif

	return true;
}