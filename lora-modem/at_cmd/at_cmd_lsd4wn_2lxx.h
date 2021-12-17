
#ifndef __AT_CMD_LSD4WN_2LXX__
#define __AT_CMD_LSD4WN_2LXX__

#define LSD4WN_2L_AT_CMD_SET_SYS_BAUD_9600               "AT+BAUD=9600\r\n"
#define LSD4WN_2L_AT_CMD_GET_SYS_BAUD                    "AT+BAUD?\r\n"
#define LSD4WN_2L_AT_CMD_RST_SYS_RESET                   "AT+RESET\r\n"
#define LSD4WN_2L_AT_CMD_RST_SYS_SAVE                    "AT+SAVE\r\n"

#define LSD4WN_2L_AT_CMD_SET_MAC_CLASS_A                 "AT+CLASS=0\r\n"
#define LSD4WN_2L_AT_CMD_SET_MAC_CLASS_B                 "AT+CLASS=1\r\n"
#define LSD4WN_2L_AT_CMD_SET_MAC_CLASS_C                 "AT+CLASS=2\r\n"
#define LSD4WN_2L_AT_CMD_GET_MAC_CLASS                   "AT+CLASS?\r\n"

#define LSD4WN_2L_AT_CMD_SET_MAC_OTAA                    "AT+OTAA=1\r\n"
#define LSD4WN_2L_AT_CMD_SET_MAC_OTAA_HOT_START          "AT+OTAA=1,1\r\n"
#define LSD4WN_2L_AT_CMD_SET_MAC_ABP                     "AT+OTAA=0\r\n"

#define LSD4WN_2L_AT_CMD_SET_MAC_CONFIRM                 "AT+CONFIRM=1\r\n"
#define LSD4WN_2L_AT_CMD_SET_MAC_UNCONFIRM               "AT+CONFIRM=0\r\n"

#define LSD4WN_2L_AT_CMD_SET_MAC_CLASS_C                 "AT+CLASS=2\r\n"
#define LSD4WN_2L_AT_CMD_GET_MAC_CLASS                   "AT+CLASS?\r\n"

#define LSD4WN_2L_AT_CMD_SET_MAC_APPEUI                  "AT+APPEUI=%s\r\n"
#define LSD4WN_2L_AT_CMD_SET_MAC_APPEUI                  "AT+APPEUI?\r\n"

#define LSD4WN_2L_AT_CMD_GET_MAC_DEVADDR                 "AT+DEVADDR?\r\n"
#define LSD4WN_2L_AT_CMD_SET_MAC_APPEUI                  "AT+APPEUI=%s\r\n"

#define LSD4WN_2L_AT_CMD_GET_MAC_DEVEUI                  "AT+DEVEUI?\r\n"
#define LSD4WN_2L_AT_CMD_FMT_SET_MAC_APPKEY              "AT+APPKEY=%s\r\n"

#define LSD4WN_2L_AT_CMD_SET_MAC_BAND_CLAA_MODE_D        "AT+BAND=4\r\n"
#define LSD4WN_2L_AT_CMD_SET_MAC_BAND_CLAA_MODE_E        "AT+BAND=5\r\n"
#define LSD4WN_2L_AT_CMD_SET_MAC_BAND_CN470              "AT+BAND=6\r\n"
#define LSD4WN_2L_AT_CMD_SET_MAC_BAND_CN470S             "AT+BAND=7\r\n"




#endif

