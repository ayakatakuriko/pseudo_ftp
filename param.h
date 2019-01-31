#ifndef PARAM_H
#define PARAM_H

#define DATASIZE 1024

/* データがある場合のパケット*/
struct myftph_data {
        uint8_t type;
        uint8_t code;
        uint16_t length;
        char data[DATASIZE];
};

/* データが無い場合のパケット*/
struct myftph {
        uint8_t type;
        uint8_t code;
        uint16_t length;
};

/* メッセージType*/
#define T_QUIT 0x01
#define T_PWD 0x02
#define T_CWD 0x03
#define T_LIST 0x04
#define T_RETR 0x05
#define T_STOR 0x06
#define T_OK 0x10
#define T_CMD_ERR 0x11
#define T_FILE_ERR 0x12
#define T_UNKNOWN_ERR 0x13
#define T_DATA 0x20

/* メッセージCode*/
#define C_OK 0x00
#define C_OK_STOC 0x01
#define C_OK_CTOS 0x02
#define C_SYNTAX_ERR 0x01
#define C_UNKNOWN_CMD 0x02
#define C_PROTOCOL_ERR 0x03
#define C_FILE_NOT_EXIST 0x00
#define C_PERMISSION_DENID 0x01
#define C_UNKNOWN_ERR 0x05
#define C_DATA_HAS_NEXT 0x00
#define C_DATA_END 0x01

#endif
