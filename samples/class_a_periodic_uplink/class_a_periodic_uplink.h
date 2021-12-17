
#include <rthw.h>
#include <rtthread.h>
#include <at.h>

#include "lora_modem.h"

#ifndef __CLASS_A_PERIODIC_UPLINK__
#define __CLASS_A_PERIODIC_UPLINK__

typedef struct app_packet_tag
{
    uint8_t cmd;
    uint32_t up_fcnt;
    uint8_t payload[217];
}app_packet_t;

#endif
