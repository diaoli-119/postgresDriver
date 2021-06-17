#pragma once

#define MESHDATA		"/home/pi/Workspace/meshData.txt"

//#define CACHE				1024*512
#define BUF_LEN				256
#define CREATED_TIME_BUF	32
#define	HEAD_STR_LEN		5
#define	HEAD_LEN			2
#define	TYPE_LEN			2
#define ENV_TEMP_LEN		4
#define ENV_HUMI_LEN		4
#define USERID_LEN			24
#define TIME_STR_LEN		26
#define	WATCHNAME_LEN		16
#define NODE_ADDR_LEN		2
#define BODY_TEMP_LEN		2
#define X_POS_LEN			4
#define Y_POS_LEN			4
#define FEVER_DATA			32
#define	RECORD_HEADER		"444E"
#define MSG_TYPE_HEADER		0x444E
#define SQLCOMMAND_LEN		512
#define NRF_SDH_BLE_GATT_MAX_MTU_SIZE	247

typedef enum
{
    MSG_TYPE_R1 = 0x5231,            //mainboard status 
    MSG_TYPE_R2 = 0x5232,            //mainboard version manuaf code
    MSG_TYPE_PO = 0x504F,            //Position msg
    MSG_TYPE_TH = 0x5448,            //SHT30 Temp&Humi Sensor msg
    MSG_TYPE_BE = 0x4245,            //Beacon Msg
    MSG_TYPE_PI = 0x5049,            //PIR Sensor msg
    MSG_TYPE_SD = 0x5344,            //SD card DATA transfer
    MSG_TYPE_Alert = 0x414C          //Set Alert broadcast
} msg_data_type_t;

typedef struct msgStruct
{
    uint16_t header;
    uint16_t type;
    uint8_t data[NRF_SDH_BLE_GATT_MAX_MTU_SIZE];
} msg_data_t;

typedef struct timeStructure
{
	uint8_t   minutes : 6;		// 0-60 (0-63 max)
	uint8_t   hours24 : 5;		// 0-23 (0-31 max)
	uint8_t   dayOfMonth : 5;	// 1-31 (0-31 max)
	uint8_t   month : 4;		// 1-12 (0-15 max)
	uint16_t  year : 12;		// Epoch start: 2014, Range: 2014 thru 4061
} po_timestamp_t;

typedef struct coorStructure
{
	uint32_t coord_x;	//coord = abs(coord)|(0xF0000000) when negtive
	uint32_t coord_y;
}  po_coord_t;

typedef struct positionStructure
{
	uint32_t UserID;			//4Bytes
	po_timestamp_t  timestamp;	//4Bytes
	po_coord_t  coord;			//8Bytes
}  position_data_t;

typedef struct shtDataStructure
{
	float sht3x_temperature;
	float sht3x_humidity;
} sht30_data_t;